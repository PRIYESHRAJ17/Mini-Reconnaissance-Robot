#include "esp_camera.h"
#include <WiFi.h>
#include "esp_http_server.h"
#include <HardwareSerial.h>

// ===== WIFI =====
const char* ssid = "YOUR_WIFI_SSID";
const char* password = "YOUR_WIFI_PASSWORD";

// ===== CAMERA PINS =====
#define PWDN_GPIO_NUM  32
#define RESET_GPIO_NUM -1
#define XCLK_GPIO_NUM   0
#define SIOD_GPIO_NUM  26
#define SIOC_GPIO_NUM  27
#define Y9_GPIO_NUM    35
#define Y8_GPIO_NUM    34
#define Y7_GPIO_NUM    39
#define Y6_GPIO_NUM    36
#define Y5_GPIO_NUM    21
#define Y4_GPIO_NUM    19
#define Y3_GPIO_NUM    18
#define Y2_GPIO_NUM     5
#define VSYNC_GPIO_NUM 25
#define HREF_GPIO_NUM  23
#define PCLK_GPIO_NUM 22

#define FLASH_LED 4

// ===== SERIAL =====
#define RXD2 14
#define TXD2 15

HardwareSerial ArduinoSerial(1);

// ===== SENSOR VALUES =====
int dr=0,dl=0,fl=0,sn=0,irR=0,irL=0,ob=0;
int ax_val=0,ay_val=0;

bool ledState=false;

// ===== HTTP SERVERS =====
httpd_handle_t cmd_httpd = NULL;
httpd_handle_t stream_httpd = NULL;

// ===== CAMERA =====
void initCamera(){

  camera_config_t config;

  config.ledc_channel = LEDC_CHANNEL_0;
  config.ledc_timer = LEDC_TIMER_0;

  config.pin_d0 = Y2_GPIO_NUM;
  config.pin_d1 = Y3_GPIO_NUM;
  config.pin_d2 = Y4_GPIO_NUM;
  config.pin_d3 = Y5_GPIO_NUM;
  config.pin_d4 = Y6_GPIO_NUM;
  config.pin_d5 = Y7_GPIO_NUM;
  config.pin_d6 = Y8_GPIO_NUM;
  config.pin_d7 = Y9_GPIO_NUM;

  config.pin_xclk = XCLK_GPIO_NUM;
  config.pin_pclk = PCLK_GPIO_NUM;
  config.pin_vsync = VSYNC_GPIO_NUM;
  config.pin_href = HREF_GPIO_NUM;

  config.pin_sscb_sda = SIOD_GPIO_NUM;
  config.pin_sscb_scl = SIOC_GPIO_NUM;

  config.pin_pwdn = PWDN_GPIO_NUM;
  config.pin_reset = RESET_GPIO_NUM;

  config.xclk_freq_hz = 20000000;

  config.pixel_format = PIXFORMAT_JPEG;

  config.frame_size = FRAMESIZE_QVGA;
  config.jpeg_quality = 12;
  config.fb_count = 2;
  config.fb_location = CAMERA_FB_IN_PSRAM;
  config.grab_mode = CAMERA_GRAB_LATEST;
  esp_camera_init(&config);
  sensor_t * s = esp_camera_sensor_get();

  s->set_brightness(s, 1);
  s->set_contrast(s, 1);
  s->set_saturation(s, 0);
  s->set_whitebal(s, 1);
  s->set_awb_gain(s, 1);
  s->set_gain_ctrl(s, 1);
  s->set_exposure_ctrl(s, 1);
}

// ===== PARSE JSON =====
int extractVal(String s,String key){

  int idx = s.indexOf(key);

  if(idx==-1) return 0;

  idx += key.length();

  int end = s.indexOf(',',idx);

  if(end==-1) end = s.indexOf('}',idx);

  if(end==-1) return 0;

  return s.substring(idx,end).toInt();
}

void parseSensorData(String s){

  dr = extractVal(s,"\"dr\":");
  dl = extractVal(s,"\"dl\":");

  fl = extractVal(s,"\"fl\":");
  sn = extractVal(s,"\"sn\":");

  irR = extractVal(s,"\"irR\":");
  irL = extractVal(s,"\"irL\":");

  ax_val = extractVal(s,"\"ax\":");
  ay_val = extractVal(s,"\"ay\":");

  ob = extractVal(s,"\"ob\":");
}

// ===== STREAM =====
esp_err_t stream_handler(httpd_req_t *req){

  camera_fb_t * fb = NULL;
  esp_err_t res = ESP_OK;

  char part_buf[64];

  httpd_resp_set_type(req, "multipart/x-mixed-replace;boundary=frame");

  while(true){

    fb = esp_camera_fb_get();

    if(!fb){
      res = ESP_FAIL;
      break;
    }

    httpd_resp_send_chunk(req, "\r\n--frame\r\n", 13);

    size_t hlen = snprintf(part_buf,64,
      "Content-Type: image/jpeg\r\nContent-Length: %u\r\n\r\n",
      fb->len
    );

    httpd_resp_send_chunk(req, part_buf, hlen);

    res = httpd_resp_send_chunk(req,(const char*)fb->buf,fb->len);

    esp_camera_fb_return(fb);

    if(res != ESP_OK){
      break;
    }
  }

  return res;
}

// ===== HTML =====
const char MAIN_page[] PROGMEM = R"=====( <!DOCTYPE html> <html> <head> <meta name="viewport" content="width=device-width, initial-scale=1"> <title>Recon Dashboard</title> <style> body{ background:#0b1220; color:white; font-family:Arial; text-align:center; margin:0; padding:0; } .top{ background:#101a2f; padding:15px; font-size:26px; font-weight:bold; color:#00d4ff; } .wrap{ padding:20px; } .openCam{ background:#00d4ff; color:black; border:none; padding:14px 20px; border-radius:10px; font-size:18px; margin-bottom:20px; } .grid{ display:grid; grid-template-columns:repeat(auto-fit,minmax(240px,1fr)); gap:15px; margin-top:20px; } .card{ background:#131f38; border-radius:14px; padding:18px; } .tt{ color:#7dd3fc; margin-bottom:10px; font-size:18px; } .val{ font-size:24px; font-weight:bold; } .ok{color:#4ade80;} .warn{color:#facc15;} .danger{color:#ef4444;} .pad{ display:grid; grid-template-columns:140px 140px 140px; gap:12px; justify-content:center; margin-top:25px; } .btn{   width:140px;
  height:95px;
  border:none;
  border-radius:18px;
  font-size:20px; background:#16233a; color:#00d4ff; } .btn:active{ background:#23406a; } .stop{ color:#ff4d4d; } .speedBox{ width:80%; margin:auto; margin-top:25px; } .bar{ width:100%; height:14px; background:#222; border-radius:10px; } .fill{ height:100%; width:70%; background:#00d4ff; border-radius:10px; } .spText{ margin-top:10px; font-size:22px; } .spBtns{ margin-top:15px; } .spBtns button{ padding:10px 18px; font-size:18px; margin:5px; border:none; border-radius:10px; } .kbd{ margin-top:25px; color:#9ca3af; } .obs{ display:none; background:#dc2626; color:white; padding:14px; margin-top:20px; border-radius:10px; font-size:20px; font-weight:bold; } .show{ display:block; } </style> </head> <body> <div class="top"> RECON ROBOT CONTROL </div> <div class="wrap"> <button class="openCam" onclick="window.open('/camera','_blank')"> OPEN CAMERA </button> <div class="obs" id="obs"> OBSTACLE DETECTED </div> <div class="speedBox"> <div class="bar"> <div class="fill" id="fill"></div> </div> <div class="spText" id="speedTxt"> 70% </div> <div class="spBtns"> <button onclick="changeSpeed(1)"> + </button> <button onclick="changeSpeed(-1)"> - </button> </div> </div> <div class="pad"> <div></div> 
<button class="btn"
onpointerdown="press('F')"
onpointerup="releaseCmd()"
onpointerleave="releaseCmd()"
onpointercancel="releaseCmd()">
FORWARD
</button> <div></div> 
<button class="btn"
onpointerdown="press('L')"
onpointerup="releaseCmd()"
onpointerleave="releaseCmd()"
onpointercancel="releaseCmd()">
LEFT
</button> <button class="btn stop" onclick="stopNow()"> STOP </button> 
<button class="btn"
onpointerdown="press('R')"
onpointerup="releaseCmd()"
onpointerleave="releaseCmd()"
onpointercancel="releaseCmd()">
RIGHT
</button> <div></div> <button class="btn"
onpointerdown="press('B')"
onpointerup="releaseCmd()"
onpointerleave="releaseCmd()"
onpointercancel="releaseCmd()">
BACKWARD
</button> <div></div> </div> <div class="kbd"> Keyboard: WASD / Arrow Keys / Space = STOP </div> <br> <label style="font-size:20px;">LED</label> <input type="checkbox" id="led" onchange="toggleLED()"> <div class="grid"> <div class="card"> <div class="tt">RIGHT DIST</div> <div class="val" id="dr">--</div> </div> <div class="card"> <div class="tt">LEFT DIST</div> <div class="val" id="dl">--</div> </div> <div class="card"> <div class="tt">FLAME</div> <div class="val" id="fl">--</div> </div> <div class="card"> <div class="tt">SOUND</div> <div class="val" id="sn">--</div> </div> <div class="card"> <div class="tt">IR RIGHT</div> <div class="val" id="irr">--</div> </div> <div class="card"> <div class="tt">IR LEFT</div> <div class="val" id="irl">--</div> </div> <div class="card">
  <div class="tt">MPU X</div>
  <div class="val" id="ax">--</div>
</div>

<div class="card">
  <div class="tt">MPU Y</div>
  <div class="val" id="ay">--</div>
</div> </div> </div> <script> let currentCmd = null; let speedStep = 7; const pwm = { 1:25, 2:51, 3:76, 4:102, 5:127, 6:153, 7:178, 8:204, 9:229, 10:255 }; function send(url){ const x = new XMLHttpRequest(); x.open("GET",url,true); x.send(); } function press(cmd){

  if(currentCmd === cmd) return;

  currentCmd = cmd;

  send('/cmd?c='+cmd);
} function releaseCmd(){

  if(currentCmd==null) return;

  currentCmd = null;

  send('/cmd?c=S');
} function stopNow(){ releaseCmd(); } document.addEventListener('mouseup',()=>{ releaseCmd(); }); document.addEventListener('pointerup',()=>{
  releaseCmd();
}); window.addEventListener('blur',()=>{ releaseCmd(); }); document.addEventListener('mouseleave',()=>{
  releaseCmd();
}); const keyMap = { 'w':'F', 'W':'F', 'ArrowUp':'F', 's':'B', 'S':'B', 'ArrowDown':'B', 'a':'L', 'A':'L', 'ArrowLeft':'L', 'd':'R', 'D':'R', 'ArrowRight':'R' }; document.addEventListener('keydown',(e)=>{ if(e.repeat) return; if(e.key===' '){ stopNow(); return; } if(e.key==='+' || e.key==='='){ changeSpeed(1); return; } if(e.key==='-'){ changeSpeed(-1); return; } const c = keyMap[e.key]; if(c){ e.preventDefault(); press(c); } }); document.addEventListener('keyup',(e)=>{ if(keyMap[e.key]){ e.preventDefault(); releaseCmd(); } }); function changeSpeed(dir){ speedStep += dir; if(speedStep < 1) speedStep = 1; if(speedStep > 10) speedStep = 10; const p = Math.round(speedStep*10); document.getElementById('speedTxt').innerText = p + "%"; document.getElementById('fill').style.width = p + "%"; send('/speed?v='+pwm[speedStep]); } function toggleLED(){ const s = document.getElementById('led').checked ? 1 : 0; send('/led?s='+s); } setInterval(()=>{ fetch('/sensors') .then(r=>r.json()) .then(d=>{ document.getElementById('dr').innerText = d.dr + " cm"; document.getElementById('dl').innerText = d.dl + " cm"; document.getElementById('fl').innerText = d.fl==1 ? "DETECTED" : "CLEAR"; document.getElementById('sn').innerText = d.sn==1 ? "DETECTED" : "CLEAR"; document.getElementById('irr').innerText = d.irR==1 ? "CLEAR" : "BLOCKED"; document.getElementById('irl').innerText = d.irL==1 ? "CLEAR" : "BLOCKED";
let axStatus = Math.abs(d.ax) > 8000 ? "TILTED" : "NORMAL";
let ayStatus = Math.abs(d.ay) > 8000 ? "TILTED" : "NORMAL";

document.getElementById('ax').innerText =
d.ax + " (" + axStatus + ")";

document.getElementById('ay').innerText =
d.ay + " (" + ayStatus + ")";
if(d.ob==1){ document.getElementById('obs').classList.add('show'); }else{ document.getElementById('obs').classList.remove('show'); } }); },300); </script> </body> </html> )=====";

// ===== HANDLERS =====
esp_err_t index_handler(httpd_req_t *req){

  httpd_resp_set_type(req,"text/html");

  return httpd_resp_send(req, MAIN_page, strlen(MAIN_page));
}

esp_err_t cmd_handler(httpd_req_t *req){

  char buf[32];

  if(httpd_req_get_url_query_str(req,buf,sizeof(buf))==ESP_OK){

    char p[8];

    if(httpd_query_key_value(buf,"c",p,sizeof(p))==ESP_OK){

      ArduinoSerial.write(p[0]);
    }
  }

  httpd_resp_send(req,"OK",2);

  return ESP_OK;
}

esp_err_t speed_handler(httpd_req_t *req){

  char buf[32];

  if(httpd_req_get_url_query_str(req,buf,sizeof(buf))==ESP_OK){

    char p[8];

    if(httpd_query_key_value(buf,"v",p,sizeof(p))==ESP_OK){

      ArduinoSerial.print('V');
      ArduinoSerial.print(atoi(p));
      ArduinoSerial.print('\n');
    }
  }

  httpd_resp_send(req,"OK",2);

  return ESP_OK;
}

esp_err_t led_handler(httpd_req_t *req){

  char buf[16];

  if(httpd_req_get_url_query_str(req,buf,sizeof(buf))==ESP_OK){

    char p[4];

    if(httpd_query_key_value(buf,"s",p,sizeof(p))==ESP_OK){

      ledState = (p[0]=='1');

      if(ledState){

  ledcWrite(FLASH_LED,255);

}else{

  ledcWrite(FLASH_LED,0);
}
    }
  }

  httpd_resp_send(req,"OK",2);

  return ESP_OK;
}

esp_err_t sensors_handler(httpd_req_t *req){

  httpd_resp_set_type(req,"application/json");

  char json[256];

  snprintf(json,sizeof(json),
  "{\"dr\":%d,\"dl\":%d,\"fl\":%d,\"sn\":%d,\"irR\":%d,\"irL\":%d,\"ax\":%d,\"ay\":%d,\"ob\":%d}",
  dr,dl,fl,sn,irR,irL,ax_val,ay_val,ob);

  return httpd_resp_send(req,json,strlen(json));
}
esp_err_t camera_page_handler(httpd_req_t *req){

  const char* page = R"rawliteral(

  <!DOCTYPE html>
  <html>

  <head>

  <meta name="viewport" content="width=device-width, initial-scale=1">

  <style>

  body{
    margin:0;
    background:black;
    text-align:center;
  }

  img{
   width:480px;
   max-width:95vw;
   border-radius:12px;
   margin-top:10px;
   image-rendering:auto;
  }

  </style>

  </head>

  <body>

<button onclick="toggleLED()"
style="
padding:12px 18px;
font-size:18px;
border:none;
border-radius:10px;
background:#00d4ff;
margin:10px;
">
FLASH
</button>

<br>

<img id="cam">

  <script>

let flashOn = false;

function toggleLED(){

  flashOn = !flashOn;

  fetch('/led?s=' + (flashOn ? '1':'0'));
}

document.getElementById('cam').src =
'http://' + location.hostname + '/stream';

</script>

  </body>
  </html>

  )rawliteral";

  httpd_resp_set_type(req,"text/html");

  return httpd_resp_send(req,page,strlen(page));
}
// ===== SERVERS =====
void startServers(){

  httpd_config_t config = HTTPD_DEFAULT_CONFIG();

  config.server_port = 80;

  httpd_uri_t index_uri = {
    .uri="/",
    .method=HTTP_GET,
    .handler=index_handler
  };

  httpd_uri_t cmd_uri = {
    .uri="/cmd",
    .method=HTTP_GET,
    .handler=cmd_handler
  };

  httpd_uri_t speed_uri = {
    .uri="/speed",
    .method=HTTP_GET,
    .handler=speed_handler
  };

  httpd_uri_t led_uri = {
    .uri="/led",
    .method=HTTP_GET,
    .handler=led_handler
  };

  httpd_uri_t sensor_uri = {
    .uri="/sensors",
    .method=HTTP_GET,
    .handler=sensors_handler
  };
  httpd_uri_t camera_uri = { .uri="/camera", .method=HTTP_GET, .handler=camera_page_handler };
  httpd_start(&cmd_httpd, &config);

  httpd_register_uri_handler(cmd_httpd,&index_uri);
  httpd_register_uri_handler(cmd_httpd,&cmd_uri);
  httpd_register_uri_handler(cmd_httpd,&speed_uri);
  httpd_register_uri_handler(cmd_httpd,&led_uri);
  httpd_register_uri_handler(cmd_httpd,&sensor_uri);
  httpd_register_uri_handler(cmd_httpd,&camera_uri);

httpd_uri_t stream_uri = {
  .uri="/stream",
  .method=HTTP_GET,
  .handler=stream_handler
};

httpd_register_uri_handler(cmd_httpd,&stream_uri);

Serial.println("STREAM ROUTE READY");

}

// ===== SETUP =====
void setup(){

  Serial.begin(115200);

ledcAttach(FLASH_LED, 5000, 8);
ledcWrite(FLASH_LED, 0);

  ArduinoSerial.begin(9600, SERIAL_8N1, RXD2, TXD2);

  initCamera();

  WiFi.begin(ssid,password);
  WiFi.setSleep(false);

  while(WiFi.status()!=WL_CONNECTED){

    delay(500);
  }

  startServers();

  Serial.println(WiFi.localIP());
}

// ===== LOOP =====
void loop(){

  static String incoming = "";

  while(ArduinoSerial.available()){

    char c = ArduinoSerial.read();

    if(c=='\n'){

      parseSensorData(incoming);

      incoming = "";
    }
    else{

      incoming += c;
    }
  }

  delay(2);
}
