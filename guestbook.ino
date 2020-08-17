#include <ESP8266WebServer.h>
#include <FS.h>
#include <DNSServer.h>

extern "C" {
#include<user_interface.h>
}

#define LOGBOOK "/logbook.txt"

//#define DEV   //devoleper mode. disable it, for faster web sites

const byte DNS_PORT = 53;
const char* user = "admin";
const char* pass = "esp8266";
const char* dns = "guestbook.com";
const char* ssid = "Guestbook";
const char* password = "";
uint8_t ip[] = {192,168,0,1};

int x, xx, counter;

unsigned long currentMillis;
unsigned long previousMillis = 0;
const long interval = 600000; //delay time for delete

String HTML, fileName, fileEdit, admin;
String Year, Month, Day, rows;
String Vname, Nname, Comment, Counter, head, nav, footer, macAddr;

String formatBytes(size_t bytes) {
  if (bytes < 1024) {
    return String(bytes) + "B";
  } else if (bytes < (1024 * 1024)) {
    return String(bytes / 1024.0) + "KB";
  } else if (bytes < (1024 * 1024 * 1024)) {
    return String(bytes / 1024.0 / 1024.0) + "MB";
  } else {
    return String(bytes / 1024.0 / 1024.0 / 1024.0) + "GB";
  }
}

bool test;

File page;
DNSServer dnsServer;
// Create an instance of the server on Port 80
ESP8266WebServer server(80);

String getContentType(String filename);
void notFound();// Sendet "Not Found"-Seite
void handleUnknown();   // Liefert Web-Seiten aus dem SPIFFS
void handleFileUpload();// upload a new file to the SPIFFS
void adminHtml();
void logHtml();
void indexHtml();
void aboutHtml();
void getMacAddr();

void setup()
{ //Serial.begin(9600);
  SPIFFS.begin();
  //delay(100);
  //loadConfig();
  IPAddress apIP(ip[0],ip[1],ip[2],ip[3]);  // set IP-address
  WiFi.mode(WIFI_AP_STA);
  WiFi.softAPConfig(apIP, apIP, IPAddress(255, 255, 255, 0)); // subnet FF FF FF 00
  WiFi.softAP(ssid,password, 1, 0, 1);                       // ssid,pw,channel,hidden,max conections
  
  dnsServer.setTTL(300);
  dnsServer.setErrorReplyCode(DNSReplyCode::ServerFailure);
  dnsServer.start(DNS_PORT, dns, apIP);
  
  server.on("/admin", HTTP_POST,                              // if the client posts to the upload page
  []() {
    server.send(200);
  },                                // Send status 200 (OK) to tell the client we are ready to receive
  handleFileUpload                                          // Receive and save the file
           );
  server.on("/admin", []() {
    if (!server.authenticate(user, pass))
      return server.requestAuthentication();
    adminHtml();
    server.send(303);
  });
  server.on("/log.html", logHtml);
  server.on("/", indexHtml);
  server.on("/index.html", indexHtml);
  server.on("/about.html", aboutHtml);
  server.on("/site.html", siteHtml);
  server.onNotFound(handleUnknown);
  server.begin();
  
#if not defined (DEV)
  loadString();
#endif
}

void loop()
{
  // Check if a client has connected
  server.handleClient();
  dnsServer.processNextRequest();
  currentMillis = millis();
  if (currentMillis - previousMillis >= interval) {
    previousMillis = currentMillis;

    SPIFFS.remove("/comments.txt");
  }
}
// Sendet "Not Found"-Seite
void notFound()
{ HTML = F("<html><head><title>404 Not Found</title></head><body>"
           "<body><h1>Page not found</h1>"
           "<br><p>404-error</p>"
           //"<body background='/404-error.jpg'>"
           "</body></html>");
  server.send(404, "text/html", HTML);
}

// Es wird versucht, die angegebene Datei aus dem SPIFFS hochzuladen
void handleUnknown()
{ fileName = server.uri();
  page = SPIFFS.open(fileName, "r");
  if (page)
  { String contentTyp = getContentType(fileName);
    size_t sent = server.streamFile(page, contentTyp);
    page.close();
  }
  else {
    notFound();
  }
}
void logHtml()
{
  #if defined (DEV)
  loadString();
  #endif
  page = SPIFFS.open (LOGBOOK, "r" ) ;
  HTML = head;
  HTML += nav;
  HTML += "<br><br><br><center><h1>Guestbook Log</h1>";
  HTML += "<p>Here you can see all people that have found the accesspoint.</p>";
  HTML += "<br><br>";
  HTML += "<table><th>Date</th><th>Name</th><th>Comment</th>";
  test = true;
  do {
    admin = page.readStringUntil('\n');
    if (admin.length() > 0) {

      Year = admin.substring(0, 4);
      Month = admin.substring(5, 7);
      Day = admin.substring(8, 10);
      for (x = 12; x < admin.length(); x++)
      {
        if (admin.charAt(x) == '|') {
          Vname = admin.substring(11, x);
          break;
        }
      }
      for (xx = x + 1; xx < admin.length(); xx++) {
        if (admin.charAt(xx) == '|') {
          Nname = admin.substring(x + 1, xx);
          break;
        }
      }
      Comment = admin.substring(xx + 1, admin.length() - 1);
      HTML += "<tr><td>" + Month + ", " + Day + ", " + Year + "&nbsp;&nbsp;&nbsp;</td>";
      HTML += "<td>" + Vname + "&nbsp;&nbsp;&nbsp;" + Nname + "&nbsp;&nbsp;&nbsp;</td>";
      rows = "1";
      if (Comment.length() > 80) rows = "2";
      if (Comment.length() > 160) rows = "3";
      if (Comment.length() > 240) rows = "4";
      HTML += "<td><textarea rows='" + rows + "' cols='80'>" + Comment + "</textarea></td></tr>";
    } else {
      test = false;
      page.close();
    }
  } while (test);
  Counter = counter;
  HTML += "</table></center><br><br>";
  HTML += "<br><p>" + Counter + " Vistors</p>";
  //HTML+="<p>"+server.client().remoteIP().toString()+"</p>";
  //getMacAddr();
  //HTML+="<p>"+macAddr+"</p>";
  HTML += footer;
  server.send(200, "text/html", HTML);
}
void adminHtml() {
  #if defined (DEV)
  loadString();
  #endif
  if (server.hasArg("format")) {
    SPIFFS.format();
  }
  if (server.hasArg("message")) {
    admin = server.arg(0).c_str();
    admin.replace("%0D%0A++", "\n");
    page = SPIFFS.open (fileEdit, "w" );
    page.print(admin);
    page.close();
  }
  if (server.hasArg("delete")) {
    fileName = server.arg(0).c_str();
    fileName = "/" + fileName;
    SPIFFS.remove(fileName);
  }
  if (server.hasArg("reboot")) {
    ESP.restart();
  }
  if (server.hasArg("show")) {
    fileEdit = server.arg(0).c_str();
    fileEdit = "/" + fileEdit;
    page = SPIFFS.open (fileEdit, "r" );
    HTML = head;
    HTML += nav;
    HTML += "<center><h1>Edit&nbsp;" + fileEdit.substring(1) + "</h1>";
    HTML += "<pre><form action='/admin'><br><textarea rows='20' cols='80' name='message'>";
    admin = page.readString();
    admin.replace("<", "&lt;");
    admin.replace(">", "&gt;");
    HTML += admin + "</textarea></pre>";
    page.close();
    HTML += "<br><input type='submit' value='save'></form><br><form action='/admin'><br><input type='submit' value='back'></form><br></center>";
    server.send(200, "text/html", HTML);
  }
  HTML = head;
  HTML += nav;
  HTML += "<center><br><br><br><form method='post' enctype='multipart/form-data'><input type='file' name='name' required><input class='button' type='submit' value='Upload'></form><br>";
  HTML += "<table>";
  Dir dir = SPIFFS.openDir("/");
  while (dir.next()) {
    HTML += "<tr><td>" + dir.fileName().substring(1) + "</td>&nbsp;&nbsp;&nbsp;";
    //HTML+="<td><form action='/admin'><input type='submit' name='show' value='"+dir.fileName().substring(1)+"'></form>";
    HTML += "<td><button><a href=admin?show=" + dir.fileName().substring(1) + ">show/edit</a></button></td>";
    HTML += "<td><button><a href='" + dir.fileName() + "' download='" + dir.fileName().substring(1) + "'>download</a></button></td>&nbsp;&nbsp;&nbsp;";
    HTML += "<td><button><a href=admin?delete=" + dir.fileName().substring(1) + ">delete</a></button></td>";
    HTML += "<td style='text-align:right'>" + formatBytes(dir.fileSize()) + "</td></tr>&nbsp;&nbsp;&nbsp;";
  }
  HTML += "</table><br>";
  FSInfo fsInfo;
  SPIFFS.info(fsInfo);
  HTML += "FS Bytes:&nbsp;";
  HTML += fsInfo.usedBytes;
  HTML += "&nbsp;/&nbsp;";
  HTML += fsInfo.totalBytes;
  HTML += "<br><br><button><a href=admin?reboot=''>restart</a></button><br>";
  HTML += "<br><button><a href=admin?format=''>format</a></button>";
  HTML += "</center>" + footer;
  server.send(200, "text/html", HTML);
}
void handleFileUpload() { // upload a new file to the SPIFFS
  HTTPUpload& upload = server.upload();
  if (upload.status == UPLOAD_FILE_START) {
    String filename = upload.filename;
    if (!filename.startsWith("/")) filename = "/" + filename;
    page = SPIFFS.open(filename, "w");            // Open the file for writing in SPIFFS (create if it doesn't exist)
    filename = String();
  } else if (upload.status == UPLOAD_FILE_WRITE) {
    if (page)
      page.write(upload.buf, upload.currentSize); // Write the received bytes to the file
  } else if (upload.status == UPLOAD_FILE_END) {
    if (page) {                                   // If the file was successfully created
      page.close();                               // Close the file again
      server.sendHeader("Location", "/admin");     // Redirect the client to the success page
      server.send(303);
    } else {
      server.send(500, "text/plain", "500: couldn't create file");
    }
  }
}
void indexHtml() {
#if defined (DEV)
  loadString();
  #endif
  getMacAddr();
  page = SPIFFS.open("/visitors.txt", "r");
  test = true;
  do {
    admin = page.readStringUntil('|');
      if (admin.length() > 0) {
      if (admin == macAddr) {
        page.close();
        test = false;
      }
    } else {
      test = false;
      page = SPIFFS.open("/visitors.txt", "a+");
      page.print(macAddr + "|");
      page.close();
      counter++;
    }
  } while (test);
  page = SPIFFS.open("/count.txt", "w");
  page.print(counter);
  HTML = head;
  HTML += nav;
  page = SPIFFS.open("/index", "r");
  HTML += page.readString();
  page.close();
  page = SPIFFS.open ("/count.txt", "r" );
  Counter = page.readString();
  page.close();
  HTML += "<br><p>" + Counter + " Vistors</p>";
  HTML += footer;
  server.send(200, "text/html", HTML);
}
void aboutHtml() {
  #if defined (DEV)
  loadString();
  #endif
  HTML = head;
  HTML += nav;
  page = SPIFFS.open("/about", "r");
  HTML += page.readString();
  page.close();
  Counter = counter;
  HTML += "<br><p>" + Counter + " Vistors</p>";
  HTML += footer;
  server.send(200, "text/html", HTML);
}
void siteHtml() {
  #if defined (DEV)
  loadString();
  #endif
  getMacAddr();
  page = SPIFFS.open("/comments.txt", "r");
  test = true;
  do {
    admin = page.readStringUntil('|');
    //Serial.println(admin);
    if (admin.length() > 0) {
      if (admin == macAddr) {
        page.close();
        errorRegister();
        return;
      }
    } else {
      test = false;
      page = SPIFFS.open("/comments.txt", "a+");
      page.print(macAddr + "|");
      page.close();
      previousMillis = millis();
    }
  } while (test);
  page = SPIFFS.open (LOGBOOK, "a+" );
  page.print(server.arg(0).c_str());
  page.print("|");
  String Comm = (server.arg(1).c_str());
  Comm.replace("\n", " ");
  Comm.replace("\r", " ");
  page.print(Comm);
  page.print("|");
  Comm = (server.arg(2).c_str());
  Comm.replace("\n", " ");
  Comm.replace("\r", " ");
  page.print(Comm);
  page.print("|");
  Comm = server.arg(3).c_str();
  Comm.replace("\n", " ");
  Comm.replace("\r", " ");
  page.println(Comm);
  page.close();
  HTML = head;
  HTML += nav;
  page = SPIFFS.open("/site", "r");
  HTML += page.readString();
  page.close();
  Counter = counter;
  HTML += "<br><p>" + Counter + " Vistors</p>";
  HTML += footer;
  server.send(200, "text/html", HTML);
}
void errorRegister() {
  #if defined (DEV)
  loadString();
  #endif
  HTML = head;
  HTML += nav;
  page = SPIFFS.open("/error", "r");
  HTML += page.readString();
  page.close();
  Counter = counter;
  HTML += "<br><p>" + Counter + " Vistors</p>";
  HTML += footer;
  server.send(200, "text/html", HTML);
}

void getMacAddr() {

  struct station_info *stat_info;

  stat_info = wifi_softap_get_station_info();

  macAddr = String(stat_info->bssid[0], HEX);
  macAddr += String(stat_info->bssid[1], HEX);
  macAddr += String(stat_info->bssid[2], HEX);
  macAddr += String(stat_info->bssid[3], HEX);
  macAddr += String(stat_info->bssid[4], HEX);
  macAddr += String(stat_info->bssid[5], HEX);
}
void loadString() {
  page = SPIFFS.open("/count.txt", "r");
  Counter = page.readString();
  page.close();
  counter = Counter.toInt();
  page = SPIFFS.open("/head", "r");
  head = page.readString();
  page.close();
  page = SPIFFS.open("/nav", "r");
  nav = page.readString();
  page.close();
  page = SPIFFS.open("/footer", "r");
  footer = page.readString();
  page.close();
}
String getContentType(String filename) { // convert the file extension to the MIME type
  if (filename.endsWith(".html")) return "text/html";
  else if (filename.endsWith(".css")) return "text/css";
  else if (filename.endsWith(".js")) return "application/javascript";
  else if (filename.endsWith(".ico")) return "image/x-icon";
  return "text/plain";
}
