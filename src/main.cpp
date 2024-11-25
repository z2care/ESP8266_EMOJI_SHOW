/*
 * https://github.com/me-no-dev/ESPAsyncWebServer
 * https://github.com/me-no-dev/AsyncTCP
 * https://github.com/tzapu/WiFiManager
 * https://github.com/Links2004/arduinoWebSockets
 * https://github.com/adafruit/Adafruit_NeoPixel
*/

#include "main.h"

ESP8266WebServer server(80);
WebSocketsServer webSocket = WebSocketsServer(81);
// 如果点阵出现乱码 请调整传输速率: NEO_KHZ400
Adafruit_NeoPixel pixels = Adafruit_NeoPixel(WIDTH * HEIGHT, NEOPIXEL_PIN, NEO_GRB + NEO_KHZ800);


void webSocketEvent(uint8_t num, WStype_t type, uint8_t * payload, size_t length) {
  switch (type) {
    case WStype_DISCONNECTED:
		Serial.printf("[%u] Disconnected!\n", num);
		break;
    case WStype_CONNECTED: {
        IPAddress ip = webSocket.remoteIP(num);
        Serial.printf("[%u] Connected from %d.%d.%d.%d url: %s\n", num, ip[0], ip[1], ip[2], ip[3], payload);
        // send message to client
        webSocket.sendTXT(num, "Connected");
		}
	break;
    case WStype_TEXT:
		Serial.printf("[%u] get Text[%u].\n", num, length);
		//Serial.println(payload);

		uint8_t i = 0;
		for (uint8_t y = 0; y < HEIGHT; y++) {
		for (uint8_t x = 0; x < WIDTH; x++) {
			uint8_t r = payload[i++];
			uint8_t g = payload[i++];
			uint8_t b = payload[i++];
#ifdef ZIGZAG_MATRIX
        uint16_t p = (y % 2) ? (((y + 1) * 8) - x - 1) : ((y * 8) + x);
#else
        uint16_t p = (y * 8) + x;
#endif
			pixels.setPixelColor(p, pixels.Color(r, g, b));
			//Serial.printf("(%u, %u) [%u] color: %u, %u, %u\n", x, y, p, r, g, b);
			}
		}
		pixels.show(); // This sends the updated pixel color to the hardware.
		break;
	}
}

void InitLittleFS(){
	if(LittleFS.begin()){      	               
		Serial.println("LittleFS Started.");
	} else {
		Serial.println("LittleFS Failed to Start.");
		return;
	}
	if (LittleFS.exists("/EmojiData.json")) {
		Serial.println("Found /EmojiData.json");	
	}else{
		File file = LittleFS.open("/EmojiData.json", "w");
		file.println(DATA);
		file.close();
	}
}

void setup() {
	Serial.begin(115200);
	pixels.begin();
	pixels.fill(pixels.Color(0, 0, 0));
	pixels.show();

	// 显示开机WIFI的logo
	ShowLOGO(WIFIlogo);
	WiFiManager wifiManager;
	// wifiManager.resetSettings(); //reset saved settings
	wifiManager.autoConnect(MDNS_NAME);

	// start MDNS 请访问 http://esp32.local/
	if (MDNS.begin(MDNS_NAME)) {Serial.println("MDNS responder started.");}


	// 模板函数
	server.onNotFound(handleUserRequet);
	server.on("/changeLED", changeLED);
	server.on("/saveImage", saveImage);
	server.on("/likedEMOJI", likeImage);
	server.on("/deleteEMOJI", DeleteDIY);
	server.on("/deleteLiked", DeleteLIKED);
	server.on("/getBrightness", GetBrightness);
	server.on("/setBrightness", SetBrightness);
	server.on("/RotateBtn", RotateBtn);
	server.on("/GetWiFimess", SendWiFimess);
	server.on("/DeleteWiFi", DeleteWiFi);

	server.begin();

	webSocket.begin();
	webSocket.onEvent(webSocketEvent);

	IPaddress = WiFi.localIP().toString().c_str();

	// Add service to MDNS
	MDNS.addService("http", "tcp", 80);
	MDNS.addService("ws", "tcp", 81);
	// 初始化LittleFS
	InitLittleFS();
	// 读取显示角度
	if (LittleFS.exists("/config.ini")){
		File file = LittleFS.open("/config.ini", "r");
		String angleS = file.readString();
		angle = angleS.toInt();
		file.close();
	}else{
		angle = 0;
	}

	// 读取显示亮度
	// 这里可以放在一个文件里面 懒得写了 下次再说😒 我要睡觉 困死了🤦‍♂️
	if (LittleFS.exists("/Bconfig.ini")){
		File file = LittleFS.open("/Bconfig.ini", "r");
		String BrightnessS = file.readString();
		Brightness = BrightnessS.toInt();
		file.close();
	}else{
		Brightness = 2;
	}
	// 打印ArduinoJson信息
	Serial.print("Using ArduinoJson version : ");
	Serial.println(ARDUINOJSON_VERSION);
}

unsigned long last_10sec = 0;
unsigned int counter = 0;
unsigned int IPlength = 0;
void loop() {
	unsigned long t = millis();
	webSocket.loop();
	server.handleClient();
	if ((t - last_10sec) > 1000) {
		bool ping = (counter % 2);
		int i = webSocket.connectedClients(ping);
		Serial.printf("%d Connected websocket clients ping: %d\n", i, ping);
		// 如果没有连接过客户端 则滚动显示IP地址
		if (ConnectFlag == 0){
			IPlength = IPaddress.length();
			if (counter >= IPlength){
				ShowLOGO(WIFIlogo);
				counter = 0;
			}
			if (IPaddress.charAt(counter) != '.'){
				int index = IPaddress.charAt(counter) - '0';
				ShowLOGO(num[index]);
			}else{ShowLOGO(dot);}
			counter ++ ;
		}
		last_10sec = millis();
	}
}

// 处理用户浏览器的HTTP访问
void handleUserRequet() {         
	// 获取用户请求网址信息
	String webAddress = server.uri();
	// 通过handleFileRead函数处处理用户访问
	bool fileReadOK = handleFileRead(webAddress);
	// 如果在LittleFS无法找到用户访问的资源，则回复404 (Not Found)
	if (!fileReadOK){                                                 
		server.send(404, "text/plain", "404 Not Found"); 
	}
}

//处理浏览器HTTP访问
bool handleFileRead(String path) {   
	ConnectFlag = 1; 
	if (path.endsWith("/")) {                   // 如果访问地址以"/"为结尾
		path = "/index.html";                     // 则将访问地址修改为/index.html便于LittleFS访问
	} 
	String contentType = getContentType(path);  // 获取文件类型
	if (LittleFS.exists(path)) {                     // 如果访问的文件可以在LittleFS中找到
		File file = LittleFS.open(path, "r");          // 则尝试打开该文件
		server.streamFile(file, contentType);// 并且将该文件返回给浏览器
		file.close();                                // 并且关闭文件
		return true;                                 // 返回true
	}
	return false;                                  // 如果文件未找到，则返回false
}

// 获取文件类型
String getContentType(String filename){
	if(filename.endsWith(".htm")) return "text/html";
	else if(filename.endsWith(".html")) return "text/html";
	else if(filename.endsWith(".css")) return "text/css";
	else if(filename.endsWith(".js")) return "application/javascript";
	else if(filename.endsWith(".png")) return "image/png";
	else if(filename.endsWith(".gif")) return "image/gif";
	else if(filename.endsWith(".jpg")) return "image/jpeg";
	else if(filename.endsWith(".ico")) return "image/x-icon";
	else if(filename.endsWith(".xml")) return "text/xml";
	else if(filename.endsWith(".pdf")) return "application/x-pdf";
	else if(filename.endsWith(".zip")) return "application/x-zip";
	else if(filename.endsWith(".gz")) return "application/x-gzip";
	else if(filename.endsWith(".json")) return "application/json";
	return "text/plain";
}

// 接收浏览器返回的表情信息；并控制灯带显示
void changeLED(){
	String LEDmess = server.arg("LEDmess");
	ShowLOGO(LEDmess);
	server.send(200, "text/plain", LEDmess);
}

// 接受浏览器返回的
void saveImage(){
	String Imagemess = server.arg("IMGmess");
	SaveFile("diyEmojis", Imagemess);
	server.send(200, "text/plain", "Saved");
}

// 收藏一个表情
void likeImage(){
	String Imagemess = server.arg("EMOJImess");
	SaveFile("likedEmojis", Imagemess);
	server.send(200, "text/plain", "Saved");
}

// 删除一个DIY表情
void DeleteDIY(){
	String Imagemess = server.arg("EMOJImess");
	DeleteEmoji("diyEmojis", Imagemess);
	server.send(200, "text/plain", "Deleted");
}

// 删除一个收藏的表情
void DeleteLIKED(){
	String Imagemess = server.arg("EMOJImess");
	DeleteEmoji("likedEmojis", Imagemess);
	server.send(200, "text/plain", "Deleted");
}

// 设置亮度
void SetBrightness(){
	String Brightmess = server.arg("Brightmess");
	Brightness = Brightmess.toInt();
	ShowLOGO(specialized);
	// 这里要优化⏹️⏹️⏹️⏹️⏹️⏹️
	if (LittleFS.exists("/Bconfig.ini")){LittleFS.remove("/Bconfig.ini");}
	File file = LittleFS.open("/Bconfig.ini", "w");
	String BrightnessS = String(Brightness);
	file.println(BrightnessS);
	file.close();

	server.send(200, "text/plain", "OK");
}

// 获取亮度
void GetBrightness(){
	server.send(200, "text/plain", String(Brightness));
}

// 十六进制字符转十进制int
int HexToDec(char x){	
	if (x == 'a') {return 10;}
	else if (x == 'b') {return 11;}
	else if (x == 'c') {return 12;}
	else if (x == 'd') {return 13;}
	else if (x == 'e') {return 14;}
	else if (x == 'f') {return 15;}
	else if (x == '0') {return 0;}
	else if (x == '1') {return 1;}
	else if (x == '2') {return 2;}
	else if (x == '3') {return 3;}
	else if (x == '4') {return 4;}
	else if (x == '5') {return 5;}
	else if (x == '6') {return 6;}
	else if (x == '7') {return 7;}
	else if (x == '8') {return 8;}
	else if (x == '9') {return 9;}
	else return (int) x;
	return 0;
}

// 显示特定的动画
void ShowLOGO(String LOGOmess){
	for (int i = 0; i < 64; i++){
		int colorIndex = 0;
		if (angle == 0){colorIndex = HexToDec(LOGOmess.charAt(angle0[i]));}
		else if (angle == 90){colorIndex = HexToDec(LOGOmess.charAt(angle90[i]));}
		else if (angle == 180){colorIndex = HexToDec(LOGOmess.charAt(angle180[i]));}
		else if (angle == 270){colorIndex = HexToDec(LOGOmess.charAt(angle270[i]));}
		String RGBstring = colorList[colorIndex];
		int R = RGBstring.substring(0,3).toInt();
		int G = RGBstring.substring(3,6).toInt();
		int B = RGBstring.substring(6,9).toInt();
		pixels.setPixelColor(i, pixels.Color(R, G, B));
	}
	pixels.setBrightness(Brightness);
	pixels.show();
}

// 保存数据json文件
void SaveFile(String name, String mess){
	File file = LittleFS.open("/EmojiData.json", "r");
	String a = file.readString();
	file.close();
	DynamicJsonDocument doc(10240);
	deserializeJson(doc, a);
	JsonObject obj = doc.as<JsonObject>();

	int length = doc[name].size();
	doc[name][length] = mess;

	File fileSaved = LittleFS.open("/EmojiData.json", "w");
	serializeJson(doc, fileSaved);
	fileSaved.close();
}

// 删除一个表情
void DeleteEmoji(String name, String mess){
	File file = LittleFS.open("/EmojiData.json", "r");
	String a = file.readString();
	file.close();
	DynamicJsonDocument doc(10240);
	deserializeJson(doc, a);
	JsonObject obj = doc.as<JsonObject>();

	int length = doc[name].size();
	Serial.println(length);
	for(int i=0; i < length; i++){
		if (doc[name][i] == mess){
			doc[name].remove(i);
		}
	}
	length = doc[name].size();
	Serial.println(length);

	File fileSaved = LittleFS.open("/EmojiData.json", "w");
	serializeJson(doc, fileSaved);
	fileSaved.close();
}

// 发送wifi名称
void SendWiFimess(){
	server.send(200, "text/plain", WiFi.SSID());
}

// 清除配网信息 并重启
void DeleteWiFi(){
	WiFi.disconnect(false, true);
	ESP.restart();
	server.send(200, "text/plain", "OK");
}

// 旋转屏幕方向
void RotateBtn(){
	angle += 90;
	if (angle > 270){angle = 0;}
	ShowLOGO(arrow);
	// 写入文件 掉电保存
	if (LittleFS.exists("/config.ini")){LittleFS.remove("/config.ini");}
	File file = LittleFS.open("/config.ini", "w");
	String angleS = String(angle);
	file.println(angleS);
	file.close();

	server.send(200, "text/plain", "OK");
}
