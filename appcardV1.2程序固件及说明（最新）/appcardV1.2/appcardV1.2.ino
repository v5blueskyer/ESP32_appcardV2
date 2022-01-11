//我的更改说明：V1.0程序
//移植SD2代码到esp32，引脚需改变，引脚和分辨率需改变，主要更改Arduino\libraries\TFT_eSPI\User_Setup.h以及主程序appcard.in中窗口大小
//底部空区域增加b站粉丝数，增加用于显示b站字符的中文字体，底部增加b站图标
//增加NVS读取，esp32和8266的wifi用户名和密码保存区不一样，8266直接保存在flash区不用额外处理，esp32保存在NVS区需要手动调用
//修改配网模式，改为web配网，配网页面中可输入城市代码和b站uid
//城市名称字体不全，替换全量字体
//增加自动亮度控制
//增加开机动画，目前3张，需要其他图片自行修改后编译
//预留闹钟功能，后续添加代码
//修改串口功能，去掉了城市代码设置(因为配网中已经有了)，新增常用亮度设置（自动亮度关闭时），最低亮度设置（自动亮度打开时），英语单词本轮播时间设置(分钟)
//断电情况下，按下按钮不松开，然后插上TYPE-C数据线通电，上电后大概等待1秒，屏幕上会有提示，提示停留在哪一个步骤，松开按钮就会进入该步骤，重启后生效
//按键控制功能有：
//1,自动亮度打开与关闭
//2,配网模式
//3,开机动画关闭
//4,开机动画设置为图片1（默认）
//5,开机动画设置为图片2
//6,开机动画设置为图片3
//7,底部动画设置为图片1（默认）
//8,底部动画设置为图片2
//9,闹钟打开与关闭
//10,页面选择1天气时钟（默认）
//11,页面选择2全屏数字时钟
//12,页面选择3圆盘时钟
//13,页面选择4投屏助手
//14,页面选择5英语单词本
//15,页面选择6喝水小助手
//16,页面选择7游戏flappy bird
//17,页面选择8系统信息
//18,页面选择9关于
//增加全屏数字时钟界面，按下按键可立即同步ntp时间
//增加圆盘时钟界面，按下按键可立即同步ntp时间
//增加投屏功能页面，需上位机配合
//增加英语单词本功能，支持轮播时间设置，按下按键可立即换单词和同步ntp时间,默认7990个单词。
//增加喝水小助手功能，默认8杯水，按一次增加一次，提示不同内容和显示进度，掉电不丢失，每天夜间1点10分清0重新计数。超过24次后，重启会清零。
//增加游戏flappy bird，按键操作，最高分保存记录掉电不丢失

//我的更改说明：V1.1程序
//1,增加了闹钟,在配网里面，配网完成后需要上电前按下按键选择闹钟打开模式才能使用,默认响1分钟,响铃的时候长按按键可关闭。


//我的更改说明：V1.2程序
//1,源码无变化,更新了烧录方式，不再使用麻烦的arduino手动分区，实现一键烧写。

//新增库，arduino设置正确的开发板，否则可能找不到这个Preferences.h文件。
#include <Preferences.h>
Preferences preferences; 

//新增字符串，存储wifi用户名和密码
String PrefSSID, PrefPassword; 





//ArduinoJson用V6版本，用于url解析
#include "ArduinoJson.h"

#include <TimeLib.h>
#include  <WiFi.h>
#include <HTTPClient.h>
#include <WiFiUdp.h>
#include <TFT_eSPI.h> 
#include <SPI.h>
#include <TJpg_Decoder.h>
#include <EEPROM.h>

#include "number.h"
#include "weathernum.h"


//字体文件
#include "font/bili.h"
#include "font/ZdyLwFont_20.h"
#include "font/zkyyt48.h"
#include "font/zkyyt72.h"
//图片类文件

#include "img/temperature.h"
#include "img/humidity.h"
#include "img/pangzi/i0.h"
#include "img/pangzi/i1.h"
#include "img/pangzi/i2.h"
#include "img/pangzi/i3.h"
#include "img/pangzi/i4.h"
#include "img/pangzi/i5.h"
#include "img/pangzi/i6.h"
#include "img/pangzi/i7.h"
#include "img/pangzi/i8.h"
#include "img/pangzi/i9.h"

//新增图片
//开机动画
#include "img\startlogo\chuyin1.h"
#include "img\startlogo\chuyin2.h"
#include "img\startlogo\chuyin3.h"

//底部动画
#include "img\dbgif\db1.h"
#include "img\dbgif\db2.h"
//重启动画
#include "img\reboot.h"
//杯子图片
#include "img\bzpic.h"
//喝水进度图
#include "img\waterjd.h"

//flappy bird图案
#include "img/bird01.h"

//Web配网
#include "src/SetWiFi.h"
#include "img/setWiFi_img.h"

//b站图标
#include "img/bilispace.h"

//英语单词本
#include "Englishlist.h"
#include "Englishfy.h"

//定义背光控制引脚
#define LCD_BL_PIN 22
//定义光敏电阻输入引脚
#define GL5528  32
//定义模式按钮输入引脚
#define Button  4 
//定义蜂鸣器引脚
#define PIN_BUZZER 13
#define BUZZER_FREQ 2500
#define startBeep() ledcWrite(1, 255)
#define stopBeep() ledcWrite(1,0)



//自动亮度控制 true - 打开 false - 关闭
bool AutoBright = true ; 
//自动亮度下，串口调亮度无效，仅在关闭自动亮度控制下有效。
int MiniBright=30;
//闹钟开关,0为关，1为开，默认关
int AlarmFlag=0;


//开机动画,0为关,1为图片1,以此类推,默认1
int startlogo=1;

//页面选择，默认1，不同数字调用不同页面功能
int pagenum=1;

//底部动画,1为图片1,以此类推,默认1
int dbgif=1;

//底部中间变量
int tdbgif=1;

//自动亮度控制中间变量,0为关，1为开
int brightc=1;
//闹钟控制中间变量,0为关，1为开
int alarmc=0;
//开机动画中间变量
int slogo=1;
//最低亮度中间变量
int mbright=-1;

//页面临时变量存储
int tpagenum=1;


//夜间时刻
int backLight_hour=0;



//屏幕初始化放在最前，以免写函数调用clk找不到


TFT_eSPI tft = TFT_eSPI();  // 引脚请自行配置tft_espi库中的 User_Setup.h文件
TFT_eSprite clk = TFT_eSprite(&tft);





//-------------------------------------------关于界面


void showgy()
{

    clk.loadFont(ZdyLwFont_20);
    clk.createSprite(240,22); 
    clk.fillSprite(0x0000); 
    clk.setTextDatum(CC_DATUM);
    clk.setTextColor(TFT_WHITE, 0x0000);
    clk.drawString("关于",120,11);
    clk.pushSprite(0,0);
    clk.deleteSprite();
    clk.unloadFont(); //释放加载字体资源


    clk.loadFont(ZdyLwFont_20);
    clk.createSprite(240,22); 
    clk.fillSprite(0x0000); 
    clk.setTextDatum(ML_DATUM);
    clk.setTextColor(TFT_WHITE, 0x0000);
    clk.drawString("程序版本:appcard V1.2",1,11);
    clk.pushSprite(0,24);
    clk.deleteSprite();
    clk.unloadFont(); //释放加载字体资源
    

    clk.loadFont(ZdyLwFont_20);
    clk.createSprite(240,22); 
    clk.fillSprite(0x0000); 
    clk.setTextDatum(ML_DATUM);
    clk.setTextColor(TFT_WHITE, 0x0000);
    clk.drawString("制作者:B站-明明很萌的 ",1,11);
    clk.pushSprite(0,48);
    clk.deleteSprite();
    clk.unloadFont(); //释放加载字体资源


   clk.setColorDepth(8);
   TJpgDec.setJpgScale(1);
   TJpgDec.setSwapBytes(true);
   TJpgDec.drawJpg(60,80,bilispace, sizeof(bilispace)); 

    
  
}



//-----------------------------------------系统信息页面

uint64_t chipid; 

void showespsys()
{  
  chipid=ESP.getEfuseMac();
   
    clk.loadFont(ZdyLwFont_20);
    clk.createSprite(240,22); 
    clk.fillSprite(0x0000); 
    clk.setTextDatum(CC_DATUM);
    clk.setTextColor(TFT_WHITE, 0x0000);
    clk.drawString("系统信息",120,11);
    clk.pushSprite(0,0);
    clk.deleteSprite();
    clk.unloadFont(); //释放加载字体资源


    clk.loadFont(ZdyLwFont_20);
    clk.createSprite(240,22); 
    clk.fillSprite(0x0000); 
    clk.setTextDatum(ML_DATUM);
    clk.setTextColor(TFT_WHITE, 0x0000);
    clk.drawString("工作模式:"+String(ESP.getFlashChipMode()),1,11);
    clk.pushSprite(0,24);
    clk.deleteSprite();
    clk.unloadFont(); //释放加载字体资源
    
    clk.loadFont(ZdyLwFont_20);
    clk.createSprite(240,22); 
    clk.fillSprite(0x0000); 
    clk.setTextDatum(ML_DATUM);
    clk.setTextColor(TFT_WHITE, 0x0000);
    clk.drawString("芯片类型:ESP32 WROVER ",1,11);
    clk.pushSprite(0,48);
    clk.deleteSprite();
    clk.unloadFont(); //释放加载字体资源

    clk.loadFont(ZdyLwFont_20);
    clk.createSprite(240,22); 
    clk.fillSprite(0x0000); 
    clk.setTextDatum(ML_DATUM);
    clk.setTextColor(TFT_WHITE, 0x0000);
    clk.drawString("芯片ID:"+String((uint16_t)(chipid>>32))+String((uint32_t)chipid),1,11);
    clk.pushSprite(0,72);
    clk.deleteSprite();
    clk.unloadFont(); //释放加载字体资源

    clk.loadFont(ZdyLwFont_20);
    clk.createSprite(240,22); 
    clk.fillSprite(0x0000); 
    clk.setTextDatum(ML_DATUM);
    clk.setTextColor(TFT_WHITE, 0x0000);
    clk.drawString("总内存:"+String(ESP.getHeapSize()),1,11);
    clk.pushSprite(0,96);
    clk.deleteSprite();
    clk.unloadFont(); //释放加载字体资源

    clk.loadFont(ZdyLwFont_20);
    clk.createSprite(240,22); 
    clk.fillSprite(0x0000); 
    clk.setTextDatum(ML_DATUM);
    clk.setTextColor(TFT_WHITE, 0x0000);
    clk.drawString("剩余内存:"+String(ESP.getFreeHeap()),1,11);
    clk.pushSprite(0,120);
    clk.deleteSprite();
    clk.unloadFont(); //释放加载字体资源


    clk.loadFont(ZdyLwFont_20);
    clk.createSprite(240,22); 
    clk.fillSprite(0x0000); 
    clk.setTextDatum(ML_DATUM);
    clk.setTextColor(TFT_WHITE, 0x0000);
    clk.drawString("Psram总内存:"+String(ESP.getPsramSize()),1,11);
    clk.pushSprite(0,146);
    clk.deleteSprite();
    clk.unloadFont(); //释放加载字体资源


    clk.loadFont(ZdyLwFont_20);
    clk.createSprite(240,22); 
    clk.fillSprite(0x0000); 
    clk.setTextDatum(ML_DATUM);
    clk.setTextColor(TFT_WHITE, 0x0000);
    clk.drawString("Psram可用:"+String(ESP.getFreePsram()),1,11);
    clk.pushSprite(0,170);
    clk.deleteSprite();
    clk.unloadFont(); //释放加载字体资源

    clk.loadFont(ZdyLwFont_20);
    clk.createSprite(240,22); 
    clk.fillSprite(0x0000); 
    clk.setTextDatum(ML_DATUM);
    clk.setTextColor(TFT_WHITE, 0x0000);
    clk.drawString("芯片版本号:"+String(ESP.getChipRevision()),1,11);
    clk.pushSprite(0,195);
    clk.deleteSprite();
    clk.unloadFont(); //释放加载字体资源


    clk.loadFont(ZdyLwFont_20);
    clk.createSprite(240,22); 
    clk.fillSprite(0x0000); 
    clk.setTextDatum(ML_DATUM);
    clk.setTextColor(TFT_WHITE, 0x0000);
    clk.drawString("芯片频率:"+String(ESP.getCpuFreqMHz()),1,11);
    clk.pushSprite(0,220);
    clk.deleteSprite();
    clk.unloadFont(); //释放加载字体资源


    clk.loadFont(ZdyLwFont_20);
    clk.createSprite(240,22); 
    clk.fillSprite(0x0000); 
    clk.setTextDatum(ML_DATUM);
    clk.setTextColor(TFT_WHITE, 0x0000);
    clk.drawString("芯片速度:"+String(ESP.getFlashChipSpeed()),1,11);
    clk.pushSprite(0,240);
    clk.deleteSprite();
    clk.unloadFont(); //释放加载字体资源


    clk.loadFont(ZdyLwFont_20);
    clk.createSprite(240,22); 
    clk.fillSprite(0x0000); 
    clk.setTextDatum(ML_DATUM);
    clk.setTextColor(TFT_WHITE, 0x0000);
    clk.drawString("SDK版本:"+String(ESP.getSdkVersion()),1,11);
    clk.pushSprite(0,265);
    clk.deleteSprite();
    clk.unloadFont(); //释放加载字体资源

    clk.loadFont(ZdyLwFont_20);
    clk.createSprite(240,22); 
    clk.fillSprite(0x0000); 
    clk.setTextDatum(ML_DATUM);
    clk.setTextColor(TFT_WHITE, 0x0000);
    clk.drawString("SDK版本:"+String(ESP.getSdkVersion()),1,11);
    clk.pushSprite(0,265);
    clk.deleteSprite();
    clk.unloadFont(); //释放加载字体资源


    clk.loadFont(ZdyLwFont_20);
    clk.createSprite(240,22); 
    clk.fillSprite(0x0000); 
    clk.setTextDatum(ML_DATUM);
    clk.setTextColor(TFT_WHITE, 0x0000);
    clk.drawString("程序容量:"+String(ESP.getSketchSize()),1,11);
    clk.pushSprite(0,290);
    clk.deleteSprite();
    clk.unloadFont(); //释放加载字体资源



  
}


















//-------------------------------------------flappy bird 游戏代码区

//extern uint8_t SmallFont[];
//extern uint8_t BigFont[];
//extern uint8_t SevenSegNumFont[];

//extern unsigned int bird01[0x41A]; // Bird Bitmap

//int x, y; // Variables for the coordinates where the display has been pressed

// Floppy Bird
int xP = 319;//柱子宽度
int yP = 100;//柱子高度
int yB = 50; //鸟Y坐标
int movingRate = 3;
int fallRateInt = 0;
float fallRate = 0;
int score = 0;
int lastSpeedUpScore = 0;
int highestScore;
boolean screenPressed = false;
boolean gameStarted = false;


int tmpscore=0;

// ===== initiateGame - Custom Function
void initiateGame(){


   

  //myGLCD.clrScr();
    tft.fillScreen(0x0000);
   // Blue background
  //  myGLCD.setColor(114, 198, 206);
  //  myGLCD.fillRect(0,0,319,239);


  tft.fillRect(0, 0, 320, 240, 0x867D);

  // Ground
//  myGLCD.setColor(221,216,148);
//  myGLCD.fillRect(0, 215, 319, 239);
 
  tft.fillRect(0, 215, 320, 25, 0xFFE0);


//  myGLCD.setColor(47,175,68);
//  myGLCD.fillRect(0, 205, 319, 214);

   tft.fillRect(0, 205, 320,10,0xB7E0);




  // Text
//  myGLCD.setColor(0, 0, 0);
//  myGLCD.setBackColor(221, 216, 148);
//  myGLCD.setFont(BigFont);
//  myGLCD.print("Score:",5,220);
//  myGLCD.setFont(SmallFont);
//  myGLCD.print("HowToMechatronics.com", 140, 220); 
//  myGLCD.setColor(0, 0, 0);
//  myGLCD.setBackColor(114, 198, 206);
//  myGLCD.print("Highest Score: ",5,5);
//  myGLCD.printNumI(highestScore, 120, 6);
//  myGLCD.print(">RESET<",255,5);
//  myGLCD.drawLine(0,23,319,23);
//  myGLCD.print("TAP TO START",CENTER,100);
   
    clk.loadFont(ZdyLwFont_20);
    clk.createSprite(50,22);
    clk.fillSprite(0xFFE0); 
    clk.setTextDatum(ML_DATUM);
    clk.setTextColor(0xF800, 0xFFE0);
    clk.drawString("得分:",1,11);
    clk.pushSprite(0,220);
    clk.deleteSprite();
    clk.unloadFont(); //释放加载字体资源
 
    clk.loadFont(ZdyLwFont_20);
    clk.createSprite(120,22); 
    clk.fillSprite(0x867D); 
    clk.setTextDatum(ML_DATUM);
    clk.setTextColor(0xF81F, 0x867D);
    clk.drawString("最高分:"+String(highestScore),1,11);
    clk.pushSprite(0,0);
    clk.deleteSprite();
    clk.unloadFont(); //释放加载字体资源

    clk.loadFont(ZdyLwFont_20);
    clk.createSprite(120,22); 
    clk.fillSprite(0x867D); 
    clk.setTextDatum(ML_DATUM);
    clk.setTextColor(0xFDA0, 0x867D);
    clk.drawString("Flappy  Bird",1,11);
    clk.pushSprite(100,120);
    clk.deleteSprite();
    clk.unloadFont(); //释放加载字体资源

     
     drawBird(yB); // Draws the bird
  
  // Wait until we tap the sreen
//  while (!gameStarted) {
//    if (myTouch.dataAvailable()) {
//    myTouch.read();
//    x=myTouch.getX();
//    y=myTouch.getY();        
//    // Reset higest score
//    if ((x>=250) && (x<=319) &&(y>=0) && (y<=28)) {
//    highestScore = 0;
//    myGLCD.setColor(114, 198, 206);
//   myGLCD.fillRect(120, 0, 150, 22);
//    myGLCD.setColor(0, 0, 0);
//    myGLCD.printNumI(highestScore, 120, 5);
//    } 

//    if ((x>=0) && (x<=319) &&(y>=30) && (y<=239)) {
//    gameStarted = true;
//    myGLCD.setColor(114, 198, 206);
//    myGLCD.fillRect(0, 0, 319, 32);
//    }   
//  }
//  }
  // Clears the text "TAP TO START" before the game start
//  myGLCD.setColor(114, 198, 206)
//  myGLCD.fillRect(85, 100, 235, 116);

    //重置分数功能和提示内容去除暂时不需要

     while (!gameStarted)
    {  
      
       if(digitalRead(Button) == HIGH)
       {
        
          delay(10);//需要延时防抖动
          gameStarted = true;

   
          tft.fillRect(0,0,120,22,0x867D);//游戏中屏蔽最高分记录
          tft.fillRect(100,120,120,22,0x867D);//游戏中屏蔽游戏标题
 
         
        
       }

      
    }
    
    
}
// ===== drawPlillars - Custom Function


//重绘图形消除残影函数
//x1,y1,x2,y2，uint16_t bgcolor
//定义翻转函数

void fzfillRect(int x1,int y1,int x2,int y2,uint16_t fillcolor)
{
     if (x1>x2)
  {
    int tempdd1=x1;
    x1=x2;
    x2=tempdd1;
    
  }
  if (y1>y2)
  {
    int tempdd2=y1;
    y1=y2;
    y2=tempdd2;
  }
            
      tft.fillRect(x1,y1,abs(x2-x1)+1,abs(y2-y1)+1,fillcolor);
}

void fzdrawRect(int x1,int y1,int x2,int y2,uint16_t drawcolor)
{

      if (x1>x2)
  {
    int tempdd1=x1;
    x1=x2;
    x2=tempdd1;
    
  }
  if (y1>y2)
  {
    int tempdd2=y1;
    y1=y2;
    y2=tempdd2;
  }
            
            
   tft.drawRect(x1,y1,abs(x2-x1)+1,abs(y2-y1)+1,drawcolor);

}




void drawPilars(int x, int y) {

   
    if (x>=270){

//      myGLCD.setColor(0, 200, 20);
//       myGLCD.fillRect(318, 0, x, y-1);
//       myGLCD.setColor(0, 0, 0);
//       myGLCD.drawRect(319, 0, x-1, y);



fzfillRect(318,0,x,y-1,0x07E0);
fzdrawRect(319,0,x-1,y,0x0000);

//       myGLCD.setColor(0, 200, 20);
//       myGLCD.fillRect(318, y+81, x, 203);
//       myGLCD.setColor(0, 0, 0);
//       myGLCD.drawRect(319, y+80, x-1, 204); 
 
fzfillRect(318,y+81,x,203,0x07E0);
fzdrawRect(319,y+80,x-1,204,0x0000);
 
    }
    else if( x<=268) {
       
      // Draws blue rectangle right of the pillar
//       myGLCD.setColor(114, 198, 206);
//       myGLCD.fillRect(x+51, 0, x+60, y);


  fzfillRect(x+51,0,x+60,y,0x867D);

      // Draws the pillar
//       myGLCD.setColor(0, 200, 20);
//       myGLCD.fillRect(x+49, 1, x+1, y-1);

 fzfillRect(x+49,1,x+1,y-1,0x07E0);

      // Draws the black frame of the pillar
//       myGLCD.setColor(0, 0, 0);
//       myGLCD.drawRect(x+50, 0, x, y);


 fzdrawRect(x+50,0,x,y,0x0000);


      // Draws the blue rectangle left of the pillar
//       myGLCD.setColor(114, 198, 206);
//       myGLCD.fillRect(x-1, 0, x-3, y);


 fzfillRect(x-1,0,x-3,y,0x867D);

      // The bottom pillar
//       myGLCD.setColor(114, 198, 206);
//       myGLCD.fillRect(x+51, y+80, x+60, 204);


  fzfillRect(x+51,y+80,x+60,204,0x867D);
 
//       myGLCD.setColor(0, 200, 20);
//       myGLCD.fillRect(x+49, y+81, x+1, 203);


  
    fzfillRect(x+49,y+81,x+1,203,0x07E0);



//       myGLCD.setColor(0, 0, 0);
//       myGLCD.drawRect(x+50,y+80,x, 204);
  
   
   

   fzdrawRect(x+50,y+80,x,204,0x0000);

//       myGLCD.setColor(114, 198, 206);
//       myGLCD.fillRect(x-1, y+80, x-3, 204);



 
      fzfillRect(x-1,y+80,x-3,204,0x867D);

  }
  // Draws the score
//   myGLCD.setColor(0, 0, 0);
//   myGLCD.setBackColor(221, 216, 148);
//   myGLCD.setFont(BigFont);
//   myGLCD.printNumI(score, 100, 220);
    
    
    clk.loadFont(ZdyLwFont_20);
    clk.createSprite(30,22);
    clk.fillSprite(0xFFE0); 
    clk.setTextDatum(ML_DATUM);
    clk.setTextColor(0xF800, 0xFFE0);
    clk.drawString(String(score),1,11);
    clk.pushSprite(52,220);
    clk.deleteSprite();
    clk.unloadFont(); //释放加载字体资源



}

//====== drawBird() - Custom Function

//  clk.drawRoundRect(0,0,200,16,8,0xFFFF);       //空心圆角矩形示例
//  clk.fillRoundRect(3,3,loadNum,10,5,0xFDA0);   //实心圆角矩形示例


void drawBird(int y) {
  // Draws the bird - bitmap
//  myGLCD.drawBitmap (50, y, 35, 30, bird01);
//  // Draws blue rectangles above and below the bird in order to clear its previus state
//  myGLCD.setColor(114, 198, 206);
//  myGLCD.fillRoundRect(50,y,85,y-6);
//  myGLCD.fillRoundRect(50,y+30,85,y+36);
//原来是位图，可能有问题，先修改再看
  
  clk.setColorDepth(8);
  TJpgDec.setJpgScale(1);
  TJpgDec.setSwapBytes(true);
  TJpgDec.drawJpg(50,y,bird01, sizeof(bird01)); //
  
//tft.fillRect(50,y,35,y-6,0x867D);
//  tft.fillRect(50,y+30,35,y+36,0x867D);

 //   tft.drawRect(50,y,35,30,0xFFFF);
    
    tft.fillRect(50,y-5,30,6,0x867D);
    tft.fillRect(50,y+24,30,6,0x867D);
    
}
//======== gameOver() - Custom Function
void gameOver() {
   
   delay(3000); // 1 second
  // Clears the screen and prints the text
//  myGLCD.clrScr();
//  myGLCD.setColor(255, 255, 255);
//  myGLCD.setBackColor(0, 0, 0);
//  myGLCD.setFont(BigFont);
//  myGLCD.print("GAME OVER", CENTER, 40);
//  myGLCD.print("Score:", 100, 80);
//  myGLCD.printNumI(score,200, 80);
//  myGLCD.print("Restarting...", CENTER, 120);
//  myGLCD.setFont(SevenSegNumFont);
//  myGLCD.printNumI(2,CENTER, 150);
//  delay(1000);
//  myGLCD.printNumI(1,CENTER, 150);
//  delay(1000);

  
    tft.fillScreen(0x0000);

    clk.loadFont(ZdyLwFont_20);
    clk.createSprite(320,22); 
    clk.fillSprite(0x0000); 
    clk.setTextDatum(CC_DATUM);
    clk.setTextColor(0xFEA0, 0x0000);
    clk.drawString("游戏结束:",160,11);
    clk.pushSprite(0,40);
    clk.deleteSprite();
    clk.unloadFont(); //释放加载字体资源

 
    clk.loadFont(ZdyLwFont_20);
    clk.createSprite(320,22); 
    clk.fillSprite(0x0000); 
    clk.setTextDatum(CC_DATUM);
    clk.setTextColor(0xFEA0, 0x0000);
    clk.drawString("得分:"+String(score),160,11);
    clk.pushSprite(0,80);
    clk.deleteSprite();
    clk.unloadFont(); //释放加载字体资源


    clk.loadFont(ZdyLwFont_20);
    clk.createSprite(320,22); 
    clk.fillSprite(0x0000); 
    clk.setTextDatum(CC_DATUM);
    clk.setTextColor(0xFEA0, 0x0000);
    clk.drawString("重启游戏中",160,11);
    clk.pushSprite(0,120);
    clk.deleteSprite();
    clk.unloadFont(); //释放加载字体资源

  
     
    clk.loadFont(ZdyLwFont_20);
    clk.createSprite(320,22); 
    clk.fillSprite(0x0000); 
    clk.setTextDatum(CC_DATUM);
    clk.setTextColor(0xFEA0, 0x0000);
    clk.drawString("2",160,11);
    clk.pushSprite(0,160);
    clk.deleteSprite();
    clk.unloadFont(); //释放加载字体资源

    delay(1000);

    clk.loadFont(ZdyLwFont_20);
    clk.createSprite(320,22); 
    clk.fillSprite(0x0000); 
    clk.setTextDatum(CC_DATUM);
    clk.setTextColor(0xFEA0, 0x0000);
    clk.drawString("1",160,11);
    clk.pushSprite(0,160);
    clk.deleteSprite();
    clk.unloadFont(); //释放加载字体资源

    delay(1000);
    
  
  // Writes the highest score in the EEPROM
  if (score > highestScore) {
    highestScore = score;
    EEPROM.write(71,highestScore);
    EEPROM.commit();
  }
  // Resets the variables to start position values
  xP=319;
  yB=50;
  fallRate=0;
  score = 0;
  lastSpeedUpScore = 0;
  movingRate = 3;  
  gameStarted = false;
  // Restart game
  initiateGame();
 
}


//---------------------------------------------------------------------

//喝水助手，默认8杯水，按键一次表示喝完一杯，每天0点清0

//已经喝水杯数
int waternum=0;


 

//临时喝水次数，用于eeprom读取
int tmpwaternum=0;

//清0函数

void clearwaternum()
{
  if(hour()==1 and minute()==15 and second()==0 )
  {
    tmpwaternum=0;
    waternum=0;
    EEPROM.write(61,0);
    EEPROM.commit(); 
    watershow();
  }
  
}





//喝水次数提示信息函数，根据次数的入参给出不同提示内容
void watertips(int bzcs)
{


    clk.loadFont(ZdyLwFont_20);
    clk.createSprite(240,24); 
    clk.fillSprite(0x0000); 
    clk.setTextDatum(ML_DATUM);
    clk.setTextColor(0xFFE0, 0x0000);
    clk.drawString("今天已喝水杯数:"+String(bzcs),1,12);
    clk.pushSprite(0,225);
    clk.deleteSprite();
    clk.unloadFont(); //释放加载字体资源

 

    if(bzcs==0)
    {
    clk.loadFont(ZdyLwFont_20);
    clk.createSprite(240,24); 
    clk.fillSprite(0x0000); 
    clk.setTextDatum(CC_DATUM);
    clk.setTextColor(0xF800, 0x0000);
    clk.drawString("注意补充水分哦",120,12);
    clk.pushSprite(0,255);
    clk.deleteSprite();
    clk.unloadFont(); //释放加载字体资源
    }

    else if(bzcs>=1 and bzcs<5)
   {
    clk.loadFont(ZdyLwFont_20);
    clk.createSprite(240,24); 
    clk.fillSprite(0x0000); 
    clk.setTextDatum(CC_DATUM);
    clk.setTextColor(0x07E0, 0x0000);
    clk.drawString("不错，继续保持",120,12);
    clk.pushSprite(0,255);
    clk.deleteSprite();
    clk.unloadFont(); //释放加载字体资源
   }
    else if(bzcs>=5 and bzcs<8)
    {

    clk.loadFont(ZdyLwFont_20);
    clk.createSprite(240,24); 
    clk.fillSprite(0x0000); 
    clk.setTextDatum(CC_DATUM);
    clk.setTextColor(0xFDA0, 0x0000);
    clk.drawString("加油，争取完成",120,12);
    clk.pushSprite(0,255);
    clk.deleteSprite();
    clk.unloadFont(); //释放加载字体资源      
    }

    else if(bzcs==8)
   { 
    clk.loadFont(ZdyLwFont_20);
    clk.createSprite(240,24); 
    clk.fillSprite(0x0000); 
    clk.setTextDatum(CC_DATUM);
    clk.setTextColor(0x780F, 0x0000);
    clk.drawString("太棒了，果然你最秀",120,12);
    clk.pushSprite(0,255);
    clk.deleteSprite();
    clk.unloadFont(); //释放加载字体资源      
    }

    else if(bzcs>8 and bzcs <=24)
    {
    clk.loadFont(ZdyLwFont_20);
    clk.createSprite(240,24); 
    clk.fillSprite(0x0000); 
    clk.setTextDatum(CC_DATUM);
    clk.setTextColor(0xFE19, 0x0000);
    clk.drawString("目标完成，喝太多水不宜哦",120,12);
    clk.pushSprite(0,255);
    clk.deleteSprite();
    clk.unloadFont(); //释放加载字体资源      
      
    }


    else if(bzcs>24)
    {
    clk.loadFont(ZdyLwFont_20);
    clk.createSprite(240,24); 
    clk.fillSprite(0x0000); 
    clk.setTextDatum(CC_DATUM);
    clk.setTextColor(0xFE19, 0x0000);
    clk.drawString("错误操作，请停止",120,12);
    clk.pushSprite(0,255);
    clk.deleteSprite();
    clk.unloadFont(); //释放加载字体资源      
      
    }
    
  
}



//喝水助手函数
void watershow()
{
   
    tft.fillScreen(TFT_BLACK);//清屏
    clk.loadFont(ZdyLwFont_20);
    clk.createSprite(240,24); 
    clk.fillSprite(0x0000); 
    clk.setTextDatum(CC_DATUM);
    clk.setTextColor(0x9A60, 0x0000);
    clk.drawString("喝水小助手",120,12);
    clk.pushSprite(0,0);
    clk.deleteSprite();
    clk.unloadFont(); //释放加载字体资源
    clk.setColorDepth(8);
    TJpgDec.setJpgScale(1);
    TJpgDec.setSwapBytes(true);
    TJpgDec.drawJpg(60,30,bzpic, sizeof(bzpic));


    
   if(waternum==0)
   {
       TJpgDec.drawJpg(60,130,waterjd0, sizeof(waterjd0));
       watertips(0);
   }
   else if(waternum==1)
   {
      TJpgDec.drawJpg(60,130,waterjd1, sizeof(waterjd1));  
      watertips(1);  
   }
   else if(waternum==2)
   {
      TJpgDec.drawJpg(60,130,waterjd2, sizeof(waterjd2)); 
      watertips(2);   
   }
   else if(waternum==3)
   {
      TJpgDec.drawJpg(60,130,waterjd3, sizeof(waterjd3)); 
      watertips(3);   
   }   
   else if(waternum==4)
   {
      TJpgDec.drawJpg(60,130,waterjd4, sizeof(waterjd4));
      watertips(4);    
   } 
   else if(waternum==5)
   {
      TJpgDec.drawJpg(60,130,waterjd5, sizeof(waterjd5)); 
      watertips(5);   
   }
   else if(waternum==6)
   {
      TJpgDec.drawJpg(60,130,waterjd6, sizeof(waterjd6)); 
      watertips(6);   
   }

   else if(waternum==7)
   {
      TJpgDec.drawJpg(60,130,waterjd7, sizeof(waterjd7)); 
      watertips(7);   
   }


   else if(waternum==8)
   {
      TJpgDec.drawJpg(60,130,waterjd8, sizeof(waterjd8)); 
      watertips(8);   
   }


   else if(waternum>8 and waternum<=24 )
   {
     TJpgDec.drawJpg(60,130,waterjdp, sizeof(waterjdp));
     watertips(waternum); 
   }

    else if(waternum>24 )
   {
     TJpgDec.drawJpg(60,130,waterjdp, sizeof(waterjdp));
     watertips(waternum); 
   }

     
}





//重启函数
void doreboot()
{
  tft.fillScreen(TFT_BLACK);//清屏
  clk.setColorDepth(8);
  TJpgDec.setJpgScale(1);
  TJpgDec.setSwapBytes(true);
     TJpgDec.drawJpg(67,71,reboot0, sizeof(reboot0));
     delay(80);
     TJpgDec.drawJpg(67,71,reboot1, sizeof(reboot1));
     delay(80);
     TJpgDec.drawJpg(67,71,reboot2, sizeof(reboot2));
     delay(80);
     TJpgDec.drawJpg(67,71,reboot3, sizeof(reboot3));
     delay(80);
     TJpgDec.drawJpg(67,71,reboot4, sizeof(reboot4));
     delay(80);
     TJpgDec.drawJpg(67,71,reboot5, sizeof(reboot5));
     delay(80);
     TJpgDec.drawJpg(67,71,reboot6, sizeof(reboot6));
     delay(80);
     TJpgDec.drawJpg(67,71,reboot7, sizeof(reboot7));
     delay(80);
     TJpgDec.drawJpg(67,71,reboot8, sizeof(reboot8));
     delay(80);
     TJpgDec.drawJpg(67,71,reboot9, sizeof(reboot9));
     delay(80);
     TJpgDec.drawJpg(67,71,reboot10, sizeof(reboot10));
     delay(80);
     TJpgDec.drawJpg(67,71,reboot11, sizeof(reboot11));
               
  clk.loadFont(ZdyLwFont_20);
  clk.createSprite(240,30); 
  clk.fillSprite(0x0000); 
  clk.setTextDatum(CC_DATUM);
  clk.setTextColor(TFT_WHITE, 0x0000);
  clk.drawString("正在重启中......",120,15);
  clk.pushSprite(0,260);
  clk.deleteSprite();
  clk.unloadFont(); //释放加载字体资源
  delay(1000);
  tft.fillScreen(TFT_BLACK);//清屏
  ESP.restart();

}

//----------------------------------页面5英语单词本部分//

//定义数组下标变量,范围从0到7990，包含0，不包含7990
int Englishindex=3455;

//定义一个时间变量，设置单词本需要刷新的时间，默认分钟
//最小单位为60000，即1分钟
int Englishfreshtime=1;
//转化分钟方便用户设置
int Truefreshtime=Englishfreshtime*60000;

//临时变量，用于eeprom读取设置

int TmpEnglishtime=1 ;

//英语单词本，随机索引数组下标并显示出来，不用重复考虑
void showEnglish()

{

    //生成随机数，包含0，不包含7990
    Englishindex=random(0,7990);
    clk.loadFont(ZdyLwFont_20);
    clk.createSprite(240,30); 
    clk.fillSprite(0x0000); 
    clk.setTextDatum(ML_DATUM);
    clk.setTextColor(TFT_WHITE, 0x0000);
    clk.drawString("英文:",5,15);
    clk.pushSprite(0,10);
    clk.deleteSprite();
    clk.unloadFont(); //释放加载字体资源

    clk.loadFont(ZdyLwFont_20);
    clk.createSprite(240,30);
    clk.fillSprite(0x0000); 
    clk.setTextDatum(ML_DATUM);
    clk.setTextColor(TFT_WHITE, 0x0000);
    clk.drawString(Englishlist[Englishindex],10,15);
    clk.pushSprite(0,50);
    clk.deleteSprite();
    clk.unloadFont(); //释放加载字体资源

    clk.loadFont(ZdyLwFont_20);
    clk.createSprite(240,30); 
    clk.fillSprite(0x0000); 
    clk.setTextDatum(ML_DATUM);
    clk.setTextColor(TFT_WHITE, 0x0000);
    clk.drawString("翻译:",5,15);
    clk.pushSprite(0,100);
    clk.deleteSprite();
    clk.unloadFont(); //释放加载字体资源

    clk.loadFont(ZdyLwFont_20);
    clk.createSprite(240,30);
    clk.fillSprite(0x0000);  
    clk.setTextDatum(ML_DATUM);
    clk.setTextColor(TFT_WHITE, 0x0000);
    clk.drawString(Englishfy[Englishindex],10,15);
    clk.pushSprite(0,140);
    clk.deleteSprite();
    clk.unloadFont(); //释放加载字体资源
}

//闹钟函数及变量-------------------------------------------------------

      int rtimeh=0;
      int rtimem=0;
      int rtimes=0;
      int sysnzrtimeh=0;
      int sysnzrtimem=0;
      int sysnzrtimes=0;
 
//闹钟使能标志,0关闭,1打开
int alarmgood =0;




//蜂鸣器闹钟模拟，默认响60次循环
int fmqcs=60;
void alarmnz()
{
  if(AlarmFlag==1 and alarmgood==1 )
  {
   for(int i=0;i<=60;i++)
   {
    startBeep();
    delay(500);
    stopBeep();
    delay(500);
     if( digitalRead(Button) == HIGH )
   {
    break;
   }
    
   }
  }
  
}

void alarmoff()
{
 stopBeep();
 alarmgood==0;

}

void sysnzmode() //

{     
   

      rtimeh=int(hour());
      rtimem=int(minute());
      rtimes=int(second());

   
    if( sysnzrtimeh== rtimeh and  sysnzrtimem==rtimem and    sysnzrtimes==  rtimes )
  {  
    alarmgood=1;
    alarmnz();
  }

}





//-----------------------------------------页面3圆盘时钟



//圆盘时钟变量
float sx = 0, sy = 1, mx = 1, my = 0, hx = -1, hy = 0;    // Saved H, M, S x & y multipliers
float sdeg=0, mdeg=0, hdeg=0;
uint16_t osx=120, osy=120, omx=120, omy=120, ohx=120, ohy=120;  // Saved H, M, S x & y coords
uint16_t x0=0, x1=0, yy0=0, yy1=0;
uint32_t targetTimey = 0;                    // for next 1 second timeout

static uint8_t conv2d(const char* p); // Forward declaration needed for IDE 1.6.x
uint8_t hh=conv2d(__TIME__), mm=conv2d(__TIME__+3), ss=conv2d(__TIME__+6);  // Get H, M, S from compile time

boolean initial = 1;


static uint8_t conv2d(const char* p) {
  uint8_t v = 0;
  if ('0' <= *p && *p <= '9')
    v = *p - '0';
  return 10 * v + *++p - '0';
}


//------------------------------------------------------------页面4投屏助手
//投屏助手代码区


#include <pgmspace.h>

uint16_t  PROGMEM dmaBuffer1[32*32]; // Toggle buffer for 32*32 MCU block, 1024bytes
uint16_t  PROGMEM dmaBuffer2[32*32]; // Toggle buffer for 32*32 MCU block, 1024bytes
uint16_t* dmaBufferPtr = dmaBuffer1;
bool dmaBufferSel = 0;

//char* ssid     = "Macho"; //填写你的wifi名字，已屏蔽默认使用首次天气时钟里面的NVS区
//char* password = "123456789"; //填写你的wifi密码已屏蔽默认使用首次天气时钟里面的NVS区
int httpPort = 8081; //设置监听端口
WiFiServer servertpzs; //初始化一个服务端对象
uint8_t buff[7000] PROGMEM= {0};//每一帧的临时缓存
uint8_t img_buff[50000] PROGMEM= {0};//用于存储tcp传过来的图片
uint16_t size_count=0;//计算一帧的字节大小 


 








//强制门户Web配网
bool setWiFi_Flag = false;
void setWiFi() {
  TJpgDec.setJpgScale(1);
  TJpgDec.setSwapBytes(true);
  TJpgDec.setCallback(tft_output);
  TJpgDec.drawJpg(9,30,setWiFi_img, sizeof(setWiFi_img));

  initBasic();
  initSoftAP();
  initWebServer();
  initDNS();
  while(setWiFi_Flag == false) {
    server.handleClient();
    dnsServer.processNextRequest();
    if(WiFi.status() == WL_CONNECTED) {
      server.stop();
      setWiFi_Flag = true;
    }
  }
}





//光映射函数，将光敏电阻接受的光线阻值变化转换成数值
int Filter_Value;
#define FILTER_N 20
int Filter() {
  int i;
  int filter_sum = 0;
  int filter_max, filter_min;
  int filter_buf[FILTER_N];
  for(i = 0; i < FILTER_N; i++) {
    filter_buf[i] = analogRead(GL5528);
    delay(1);
  }
  filter_max = filter_buf[0];
  filter_min = filter_buf[0];
  filter_sum = filter_buf[0];
  for(i = FILTER_N - 1; i > 0; i--) {
    if(filter_buf[i] > filter_max)
      filter_max=filter_buf[i];
    else if(filter_buf[i] < filter_min)
      filter_min=filter_buf[i];
    filter_sum = filter_sum + filter_buf[i];
    filter_buf[i] = filter_buf[i - 1];
  }
  i = FILTER_N - 2;
  filter_sum = filter_sum - filter_max - filter_min + i / 2; // +i/2 的目的是为了四舍五入
  filter_sum = filter_sum / i;
  return filter_sum;
}



//哔哩哔哩粉丝模块
const char* host2 = "api.bilibili.com"; // B站服务器地址
String UID = "385784682";               // B站用户UID数字号，配网中可修改
int following = 0;                      // 关注数
int follower = 0;                       // 粉丝数 

//获取b站粉丝数量————————————————————————//

//调用应在wifi连接以后

void get_Bstation_follow()
{
    //创建TCP连接
    WiFiClient clientbili;
    const int httpPort = 80;
    if (!clientbili.connect(host2, httpPort))
    {
      Serial.println("Connection failed");  //网络请求无响应打印连接失败
      return;
    }
    //URL请求地址
    String url ="/x/relation/stat?vmid="+ UID + "&jsonp=jsonp"; // B站粉丝数链接，注意修改UID
    //发送网络请求
    clientbili.print(String("GET ") + url + " HTTP/1.1\r\n" +
              "Host: " + host2 + "\r\n" +
              "Connection: close\r\n\r\n");
    delay(2000);
    //定义answer变量用来存放请求网络服务器后返回的数据
    String answer;
    while(clientbili.available())
    {
      String line = clientbili.readStringUntil('\r');
      answer += line;
    }
      //断开服务器连接
    clientbili.stop();
    Serial.println();
    Serial.println("关闭连接中");
    //获得json格式的数据
    
    String jsonAnswer;
    int jsonIndex;
    //找到有用的返回数据位置i 返回头不要
    for (int i = 0; i < answer.length(); i++) {
      if (answer[i] == '{') {
        jsonIndex = i;
        break;
      }
    }
    jsonAnswer = answer.substring(jsonIndex);
    Serial.println();
    Serial.println("JSON answer: ");
    Serial.println(jsonAnswer);  
    StaticJsonDocument<256> doc;

    deserializeJson(doc, jsonAnswer);
    JsonObject data = doc["data"];
    following = data["following"]; // 59
    follower = data["follower"]; // 411
    Serial.println("UID: ");
    Serial.println(UID);
    Serial.println("Following: ");
    Serial.println(following);
    Serial.println("follower: ");
    Serial.println(follower);
    vTaskDelay(3);
}

//显示B站图标
unsigned long oldTimenew = 0,imgNumnew = 1;
void bilishow(){

   int xnew=10,ynew=242,dtnew=50;
   
  if(dbgif==1)
  {
    
  if(millis() - oldTimenew >= dtnew) {
    imgNumnew = imgNumnew + 1;
    oldTimenew = millis();
  }
   switch(imgNumnew) {
      xnew=0;
      case 1:TJpgDec.drawJpg(xnew,ynew,bili0, sizeof(bili0));break;
      case 2:TJpgDec.drawJpg(xnew,ynew,bili1, sizeof(bili1));break;
      case 3:TJpgDec.drawJpg(xnew,ynew,bili2, sizeof(bili2));break;
      case 4:TJpgDec.drawJpg(xnew,ynew,bili3, sizeof(bili3));break;
      case 5:TJpgDec.drawJpg(xnew,ynew,bili4, sizeof(bili4));break;
      case 6:TJpgDec.drawJpg(xnew,ynew,bili5, sizeof(bili5));break;
      case 7:TJpgDec.drawJpg(xnew,ynew,bili6, sizeof(bili6));imgNumnew = 0;break;
  
         }
  }

  else if(dbgif==2)
  { xnew=0;
    ynew=260;
    TJpgDec.drawJpg(xnew,ynew,bili20, sizeof(bili20));
  }
 
  vTaskDelay(3);
}



/*** Component objects ***/
Number      dig;
WeatherNum  wrat;


uint32_t targetTime = 0;   
uint16_t bgColor = 0x0000;
String cityCode = "101250101";  //天气城市代码 长沙:101250101株洲:101250301衡阳:101250401
int LCD_BL_PWM = 150;//屏幕亮度0-255
int tempnum = 0;   //温度百分比
int huminum = 0;   //湿度百分比
int tempcol =0xffff;
int humicol =0xffff;
int Anim = 0;
int prevTime = 0;
int AprevTime = 0;
int BL_addr = 1;//被写入数据的EEPROM地址编号  0亮度





//NTP服务器
static const char ntpServerName[] = "ntp6.aliyun.com";
const int timeZone = 8;     //东八区





WiFiUDP Udp;
WiFiClient wificlient;
unsigned int localPort = 8000;
float duty=0;
time_t getNtpTime();
void digitalClockDisplay();
void printDigits(int digits);
String num2str(int digits);
void sendNTPpacket(IPAddress &address);


bool tft_output(int16_t x, int16_t y, uint16_t w, uint16_t h, uint16_t* bitmap)
{
  if(pagenum==4)
{
  if ( y >= tft.height() ) return 0;
 
  // Double buffering is used, the bitmap is copied to the buffer by pushImageDMA() the
  // bitmap can then be updated by the jpeg decoder while DMA is in progress
  if (dmaBufferSel) dmaBufferPtr = dmaBuffer2;
  else dmaBufferPtr = dmaBuffer1;
  dmaBufferSel = !dmaBufferSel; // Toggle buffer selection
  //  pushImageDMA() will clip the image block at screen boundaries before initiating DMA
  tft.pushImageDMA(x, y, w, h, bitmap, dmaBufferPtr); // Initiate DMA - blocking only if last DMA is not complete
  return 1;
}
else
{
  if ( y >= tft.height() ) return 0;
  tft.pushImage(x, y, w, h, bitmap);
  // Return 1 to decode next block
  return 1;
}
}


//粉丝数量显示

void fanspush()
{
  // 粉丝数量展示界面

    clk.setColorDepth(8);
    clk.createSprite(120, 34); 
    clk.fillSprite(0X0000);
    clk.loadFont(bili);
    clk.setTextDatum(ML_DATUM);
    clk.setTextColor(TFT_MAGENTA, 0X0000);
    clk.drawString("关注数:" + String(following),1,17);
    clk.pushSprite(82,245);
    clk.deleteSprite();
    clk.unloadFont(); //释放加载字体资源


    clk.createSprite(120, 34);
    clk.fillSprite(0X0000);
    clk.loadFont(bili); 
    clk.setTextDatum(ML_DATUM);
    clk.setTextColor(TFT_MAGENTA, 0X0000);
    clk.drawString("粉丝数:" + String(follower),1,17);
    clk.pushSprite(82,280);
    clk.deleteSprite();
    clk.unloadFont(); //释放加载字体资源

    vTaskDelay(3);
  }



byte loadNum = 6;
void loading(byte delayTime)//绘制进度条
{
  clk.setColorDepth(8);
  
  clk.createSprite(200, 100);//创建窗口
  clk.fillSprite(0x0000);   //填充率

  clk.drawRoundRect(0,0,200,16,8,0xFFFF);       //空心圆角矩形
  clk.fillRoundRect(3,3,loadNum,10,5,0xFDA0);   //实心圆角矩形
  clk.setTextDatum(CC_DATUM);   //设置文本数据
  clk.setTextColor(0x867D, 0x0000); 
  clk.drawString("Connecting to WiFi......",100,40,2);
  clk.setTextColor(0xFE19, 0x0000); 
  clk.drawRightString("APPCARD V1.2",180,60,2);
  clk.pushSprite(20,110);  //窗口位置
  
  
  clk.deleteSprite();
  loadNum += 1;
  delay(delayTime);
}

void humidityWin()
{
  clk.setColorDepth(8);
  
  huminum = huminum/2;
  clk.createSprite(52, 6);  //创建窗口
  clk.fillSprite(0x0000);    //填充率
  clk.drawRoundRect(0,0,52,6,3,0xFFFF);  //空心圆角矩形  起始位x,y,长度，宽度，圆弧半径，颜色
  clk.fillRoundRect(1,1,huminum,4,2,humicol);   //实心圆角矩形
  clk.pushSprite(45,222);  //窗口位置
  clk.deleteSprite();
}
void tempWin()
{
  clk.setColorDepth(8);
  
  clk.createSprite(52, 6);  //创建窗口
  clk.fillSprite(0x0000);    //填充率
  clk.drawRoundRect(0,0,52,6,3,0xFFFF);  //空心圆角矩形  起始位x,y,长度，宽度，圆弧半径，颜色
  clk.fillRoundRect(1,1,tempnum,4,2,tempcol);   //实心圆角矩形
  clk.pushSprite(45,192);  //窗口位置
  clk.deleteSprite();
}



String SMOD = "";//0亮度

void Serial_set()//串口设置
{
  String incomingByte = "";
  if(Serial.available()>0)
  {
    
    while(Serial.available()>0)//监测串口缓存，当有数据输入时，循环赋值给incomingByte
    {
      incomingByte += char(Serial.read());//读取单个字符值，转换为字符，并按顺序一个个赋值给incomingByte
      delay(2);//不能省略，因为读取缓冲区数据需要时间
    }    
    if(AutoBright==false and SMOD=="0x01")//设置1亮度设置
    {
      int LCDBL = atoi(incomingByte.c_str());//int n = atoi(xxx.c_str());//String转int
      if(LCDBL>=0 && LCDBL<=255)
      {
        EEPROM.write(BL_addr, LCDBL);//亮度地址写入亮度值
        EEPROM.commit();//保存更改的数据
        delay(5);
        LCD_BL_PWM = EEPROM.read(BL_addr); 
        delay(5);
        SMOD = "";
        Serial.printf("亮度调整为：");
//        analogWrite(LCD_BL_PIN, 1023 - (LCD_BL_PWM*10));
//esp32的pwm语法不同
        ledcWrite(0,LCD_BL_PWM);
        Serial.println(LCD_BL_PWM);
        Serial.println("");
      }
      else
        Serial.println("亮度调整错误，请输入0-255");
    } 
    if(AutoBright==true and SMOD=="0x02")//设置2最低亮度设置
    {
      int mt = atoi(incomingByte.c_str());//int n = atoi(xxx.c_str());//String转int
      if(mt>0 && mt<150)
      {
        EEPROM.write(11,mt);//亮度地址写入亮度值
        EEPROM.commit();//保存更改的数据
        delay(5);
        SMOD = "";
        Filter_Value = Filter();
        ledcWrite(0,map(Filter_Value,MiniBright,4095,MiniBright,255));
        delay(5);
        Serial.printf("最低亮度调整为：");
        Serial.println(mt);
      }
      else
        Serial.println("最低亮度调整错误，请输入0-150");
    } 


    if( SMOD=="0x03")//设置3英语单词本刷新时间设置
    {
      int yydcbtime = atoi(incomingByte.c_str());
      if(yydcbtime>0 && yydcbtime<9999)
      {
        EEPROM.write(111,yydcbtime);//刷新时间写入
        EEPROM.commit();//保存更改的数据
        delay(5);
        SMOD = "";
        Serial.printf("英语单词本刷新时间调整为(单位分钟)：");
        Serial.println(yydcbtime);
        Serial.printf("本设置需要重启生效,系统即将重启：");
        doreboot();
      }
      else
        Serial.println("英语单词本刷新时间调整错误，请输入0-9999");
    } 

       if( SMOD=="0x04")//设置4 页面设置，方便调试
    {
      int tmppage = atoi(incomingByte.c_str());
      if(tmppage>0 && tmppage<10)
      {
        EEPROM.write(91,tmppage);//页面设置写入
        EEPROM.commit();//保存更改的数据
        delay(5);
        SMOD = "";
        Serial.printf("页面调整为：");
        Serial.println(tmppage);
        Serial.printf("本设置需要重启生效,系统即将重启：");
        doreboot();
      }
      else
        Serial.println("页面设置错误，请输入1-9");
    } 
    
    else
    {
      SMOD = incomingByte;
      delay(2);
      if(AutoBright==false and SMOD=="0x01")
        Serial.println("请输入常用亮度值，范围0-255");
      else if(AutoBright==true and SMOD=="0x02")
        Serial.println("请输入最低亮度值，范围0-150"); 
      else if(SMOD=="0x03")
        Serial.println("请输入英语单词本刷新时间（默认分钟），范围0-9999");  
      else if(SMOD=="0x04")
        Serial.println("请输入需要显示的页面，范围1-9");             

      else
      {
        Serial.println("");
        Serial.println("请输入需要修改的代码：");
        if(AutoBright==false){
           Serial.println("常用亮度设置输入    0x01"); 
          }
         if(AutoBright==true)
         { 
            Serial.println("最低亮度设置输入    0x02");
          }
         Serial.println("英语单词本刷新时间设置输入    0x03");
         Serial.println("显示的页面设置输入    0x04");
        Serial.println("");
        
      }
    }


  }
}

void setup()
{
  Serial.begin(115200);
  //设置eeprom，取值范围为4~4096字节
  EEPROM.begin(128);

//读取eeprom中的11位置的数据，最低亮度
  mbright = EEPROM.read(11);
//读取eeprom中的21位置的数据，开机动画
  slogo = EEPROM.read(21);
//读取eeprom中31位置的数据，自动亮度设置
  brightc = EEPROM.read(31);
//读取eeprom中41位置的数据，闹钟开关设置
  alarmc = EEPROM.read(41);
//读取eeprom中51位置的数据，底部动画设置
  tdbgif = EEPROM.read(51);
//读取eeprom中61位置的数据，喝水次数获取
  tmpwaternum = EEPROM.read(61);
//读取eeprom中71位置的数据，flappy bird游戏最高分
  tmpscore=EEPROM.read(71); 
//读取eeprom中91位置的数据，页面设置
  tpagenum = EEPROM.read(91);
//读取eeprom区域中111位置的数据，单词本刷新时间
  TmpEnglishtime=EEPROM.read(111);
  
if(brightc==0)
{
  AutoBright=false;
  }
else if(brightc==1)
 {
  AutoBright=true;
  } 

  
if(alarmc==0)
{
  AlarmFlag=0;
  }
else if(alarmc==1)
 {
  AlarmFlag=1;
  } 

if(slogo==0)

{
  startlogo =0;
}

else if(slogo==1)

{
  startlogo =1;
}

else if(slogo==2)
{
  startlogo =2;
}

else if(slogo==3)

{
 startlogo =3;
}

if(mbright<150 and mbright>0)
{
 MiniBright=mbright;  
}

else 
{
  MiniBright=30;
}

if(tpagenum==1)
{
  pagenum=1;
}
else if(tpagenum==2)
{
  pagenum=2;  
}

else if(tpagenum==3)
{
 pagenum=3;
}
else if(tpagenum==4)
{
 pagenum=4;
}

else if(tpagenum==5)
{
 pagenum=5;
}

else if(tpagenum==6)
{
 pagenum=6;
}
else if(tpagenum==7)
{
 pagenum=7;
}
else if(tpagenum==8)
{
 pagenum=8;
}

else if(tpagenum==9)
{
 pagenum=9;
}



if(TmpEnglishtime>0 and TmpEnglishtime<9999)
{
  Englishfreshtime=TmpEnglishtime;
}
else
{
  Englishfreshtime=1;
}

if(tdbgif==1)

{
  dbgif =1;
}

else if(tdbgif==2)

{
  dbgif =2;
}

if(tmpwaternum==1 or tmpwaternum==2  or tmpwaternum==3 or tmpwaternum==4  or tmpwaternum==5  or tmpwaternum==6  or tmpwaternum==7 or tmpwaternum==8 )
{
  waternum=tmpwaternum;
}

else if (tmpwaternum>8  and  tmpwaternum<=24)

{
  waternum=tmpwaternum;
}


else

{
  waternum=0;
}

if(tmpscore!=255)
{

  highestScore=tmpscore;
  
}

else 
{
    highestScore = 0;
}



 //mode模式按键对应esp32引脚，输入模式
  pinMode(Button,INPUT); //配网按钮接GPIO-4
//光敏电阻读取对应的esp32引脚，输入模式
  pinMode(GL5528,INPUT); //光敏电阻
//随机读取光敏电阻值
  randomSeed(analogRead(GL5528));

//ledc详细说明 
//ESP32 的 LEDC 总共有16个路通道（0 ~ 15），分为高低速两组，高速通道（0 ~ 7）由80MHz时钟驱动，低速通道（8 ~ 15）由 1MHz 时钟驱动。
//double ledcSetup(uint8_t channel, double freq, uint8_t resolution_bits)
//通道最终频率 = 时钟 / ( 分频系数 * ( 1 << 计数位数 ) );（分频系数最大为1024）
//channel 为通道号，取值0 ~ 15
//freq  期望设置频率
//resolution_bits 计数位数，取值0 ~ 20（该值决定后面 ledcWrite 方法中占空比可写值，比如该值写10，即2的n次方减去1，则占空比最大可写1023 即 (1<<resolution_bits)-1 ）



//设置esp32通道0，频率5000hz，位数8位，PWM信号
  ledcSetup(0,5000,8);
//esp32支持多通道，改语句是将0通道绑定到LCD_BL_PIN引脚上
  ledcAttachPin(LCD_BL_PIN,0);

//自动调光关闭时，读取eeprom中亮度数据
  if((EEPROM.read(BL_addr)>0&&EEPROM.read(BL_addr)<255) and AutoBright==false )
    {LCD_BL_PWM = EEPROM.read(BL_addr);
     ledcWrite(0,LCD_BL_PWM);
     }

//指定通道输出一定占空比波形
//默认初始化，写0通道，默认150，最大255，设置默认亮度
  ledcWrite(0,LCD_BL_PWM);

//蜂鸣器1通道
  ledcSetup(1, 1000, 8);
  ledcAttachPin(PIN_BUZZER, 1);
  ledcWrite(1, 0);

 
  tft.begin(); /* TFT init */
  tft.invertDisplay(0);//反转所有显示颜色：1反转，0正常
  tft.fillScreen(0x0000);
  tft.setTextColor(TFT_WHITE, bgColor);
  // 设置屏幕显示的旋转角度，参数为：0, 1, 2, 3
  // 分别代表 0°、90°、180°、270°
  //根据实际需要旋转
  tft.setRotation(0); 



  targetTime = millis() + 1000; 

   //首次使用自动进入配网模式,读取NVS存储空间内的ssid、password和citycode,biliuid
  preferences.begin("wifi", false);
  PrefSSID =  preferences.getString("ssid", "none"); 
  PrefPassword =  preferences.getString("password", "none");
  cityCode =  preferences.getString("citycode", "none");
  String tmpuid =preferences.getString("biliUID", "none");
    String nzszhhh=preferences.getString("nzhhh", "none");
      String nzszmmm=preferences.getString("nzmmm", "none");
        String nzszsss=preferences.getString("nzsss", "none");

  if(nzszhhh!=NULL)
  {
    sysnzrtimeh=atoi((nzszhhh).c_str());
  }

   if(nzszmmm!=NULL)
  {
    sysnzrtimem=atoi((nzszmmm).c_str());
  }

   if(nzszsss!=NULL)
  {
    sysnzrtimes=atoi((nzszsss).c_str());
  }
  
        
  if(tmpuid!=NULL)
  {
    UID=tmpuid;
  }

  
  preferences.end();
  if( PrefSSID == "none" )
  {
    setWiFi();
  }


  
//读取模式按键按住的时间，根据按住时间调整功能
  int buttonStateTime = 0;
  
  while(digitalRead(Button) == HIGH) {
    buttonStateTime = millis();
    clk.loadFont(ZdyLwFont_20);
    clk.createSprite(240, 80); 
    clk.setTextDatum(CC_DATUM);
    clk.setTextColor(TFT_WHITE, bgColor);
 //根据按键功能，调整显示内容


     if(buttonStateTime >= 34500) {
      clk.drawString("" + String(millis()/1000) + "秒" + " 页面9关于",120,40);
    }


  else      if(buttonStateTime >= 32500) {
      clk.drawString("" + String(millis()/1000) + "秒" + " 页面8系统信息",120,40);
    }


   else     if(buttonStateTime >= 30500) {
      clk.drawString("" + String(millis()/1000) + "秒" + " 页面7flappy bird",120,40);
    }

    

 else   if(buttonStateTime >= 28500) {
      clk.drawString("" + String(millis()/1000) + "秒" + " 页面6喝水小助手",120,40);
    }
    
 else  if(buttonStateTime >= 26500) {
      clk.drawString("" + String(millis()/1000) + "秒" + " 页面5英文单词本",120,40);
    }


 else  if(buttonStateTime >= 24500) {
      clk.drawString("" + String(millis()/1000) + "秒" + " 页面4投屏助手",120,40);
    }
    
   else if(buttonStateTime >= 22500) {
      clk.drawString("" + String(millis()/1000) + "秒" + " 页面3圆盘时钟",120,40);
    }


    else if(buttonStateTime >= 20500) {
      clk.drawString("" + String(millis()/1000) + "秒" + " 页面2全屏数字时钟",120,40);
    }

    else if(buttonStateTime >= 18500) {
      clk.drawString("" + String(millis()/1000) + "秒" + " 页面1天气时钟",120,40);
    }

    
   else if(buttonStateTime >= 16500) {
//闹钟设置取反处理，当前打开则提示关闭，关闭则提示打开   
    if(AlarmFlag==0)

    {             

      clk.drawString("" + String(millis()/1000) + "秒" + " 闹钟打开",120,40);

     }


    else if(AlarmFlag==1) 

    {             

      clk.drawString("" + String(millis()/1000) + "秒" + " 闹钟关闭",120,40);

     }

    } 

    else if(buttonStateTime >= 14500) {
      clk.drawString("" + String(millis()/1000) + "秒" + " 底部动画选择2",120,40);
    }

    else if(buttonStateTime >= 12500) {
      clk.drawString("" + String(millis()/1000) + "秒" + " 底部动画选择1",120,40);
    }

    
   else  if(buttonStateTime >= 10500) {
      clk.drawString("" + String(millis()/1000) + "秒" + " 开机动画选择3",120,40);
    } 

    else if(buttonStateTime >= 8500) {
      clk.drawString("" + String(millis()/1000) + "秒" + " 开机动画选择2",120,40);
    }

    else if(buttonStateTime >= 6500) {
      clk.drawString("" + String(millis()/1000) + "秒" + " 开机动画选择1",120,40);
    }

    else if(buttonStateTime >= 4500) {
      clk.drawString("" + String(millis()/1000) + "秒" + " 开机动画关闭",120,40);
    }
    else if(buttonStateTime >= 2500) {
      clk.drawString("" + String(millis()/1000) + "秒" + " 配网模式",120,40);
    }
    else if(buttonStateTime >= 500) {
//自动亮度取反处理，当前打开则提示关闭，关闭则提示打开   
    if(AutoBright==true)
    { 
    
     clk.drawString("" + String(millis()/1000) + "秒" + " 自动亮度关闭",120,40);

      }

   else if(AutoBright==false)
    
     { 
    
     clk.drawString("" + String(millis()/1000) + "秒" + " 自动亮度打开",120,40);

      }

  
       
      
    }
    clk.pushSprite(0,80);
    clk.deleteSprite();
    clk.unloadFont(); //释放加载字体资源
  }
  
//根据按键时间，调用对应功能


  

    if(buttonStateTime >= 34500) {
    EEPROM.write(91,9);
    EEPROM.commit();
    delay(1000);
    doreboot(); //页面9关于
    }

    
else   if(buttonStateTime >= 32500) {
    EEPROM.write(91,8);
    EEPROM.commit();
    delay(1000);
    doreboot(); //页面8系统信息
    }


else   if(buttonStateTime >= 30500) {
    EEPROM.write(91,7);
    EEPROM.commit();
    delay(1000);
    doreboot(); //页面7游戏flappy bird
    }

 else  if(buttonStateTime >= 28500) {
    EEPROM.write(91,6);
    EEPROM.commit();
    delay(1000);
    doreboot(); //页面6喝水小助手
    }
    
else   if(buttonStateTime >= 26500) {
    EEPROM.write(91,5);
    EEPROM.commit();
    delay(1000);
    doreboot(); //页面5英语单词本
    }
    


 else  if(buttonStateTime >= 24500) {
    EEPROM.write(91,4);
    EEPROM.commit();
    delay(1000);
    doreboot(); //页面4投屏助手
    }
    
   else if(buttonStateTime >= 22500) {
    EEPROM.write(91,3);
    EEPROM.commit();
    delay(1000);
    doreboot();   //页面3圆盘时钟
    }


    else if(buttonStateTime >= 20500) {
    EEPROM.write(91,2);
    EEPROM.commit();
    delay(1000);
    doreboot();//页面2全屏数字时钟
    }

    else if(buttonStateTime >= 18500) {
    EEPROM.write(91,1);
    EEPROM.commit();
    delay(1000);
    doreboot(); //页面1天气时钟
    }



  else if(buttonStateTime >= 16500) {  //闹钟控制

  if(AlarmFlag==0)
    {
         EEPROM.write(41,1);
         EEPROM.commit();
     delay(1000);
    doreboot(); 

       }
  else if(AlarmFlag==1)

 {
         EEPROM.write(41,0);
         EEPROM.commit();
    delay(1000);
    doreboot();
   }

    } 

  else if(buttonStateTime >= 14500) { //底部动画选择2
    EEPROM.write(51,2);
    EEPROM.commit();
    delay(1000);
    doreboot();
  }
  else if(buttonStateTime >= 12500) { //底部动画选择1
    EEPROM.write(51,1);
    EEPROM.commit();
    delay(1000);
    doreboot();
  }

else  if(buttonStateTime >= 10500) { //开机动画选择3
    EEPROM.write(21,3);
    EEPROM.commit();
    delay(1000);
    doreboot();
  } 
  else if(buttonStateTime >= 8500) { //开机动画选择2
    EEPROM.write(21,2);
    EEPROM.commit();
    delay(1000);
    doreboot();
  }
  else if(buttonStateTime >= 6500) { //开机动画选择1
    EEPROM.write(21,1);
    EEPROM.commit();
    delay(1000);
    doreboot();
  }
  else if(buttonStateTime >= 4500) { //开机动画关闭
    EEPROM.write(21,0);
    EEPROM.commit();
    delay(1000);
    doreboot();
  }
  else if(buttonStateTime >= 2500) { //配网模式
    setWiFi();
    delay(1000);
    doreboot();
  }
  else if(buttonStateTime >= 500) { //自动亮度控制
    if(AutoBright==true)
    { 
    
    EEPROM.write(31,0);
    EEPROM.commit();
    delay(1000);
    doreboot();
      }

   else if(AutoBright==false)
    
     { 
    
    EEPROM.write(31,1);
    EEPROM.commit();
    delay(1000);
    doreboot();
      }
  }

 





  Serial.print("正在连接WIFI ");
  Serial.println(PrefSSID);
//  WiFi.begin(ssid, pass);
  WiFi.begin(PrefSSID.c_str(), PrefPassword.c_str()); 
  TJpgDec.setJpgScale(1);
  TJpgDec.setSwapBytes(true);
  TJpgDec.setCallback(tft_output);

  while (WiFi.status() != WL_CONNECTED) 
//   while (PrefSSID == "none")
  {
    loading(70);  
      
    if(loadNum>=194)
    {
      setWiFi();
      break;
    }
  }
  delay(10); 
  while(loadNum < 194) //让动画走完
  { 
    loading(1);
  }

  Serial.println("连接成功，开始加载开机动画 ");



if( startlogo==0)
{ clk.setColorDepth(8);
  TJpgDec.setJpgScale(1);
  TJpgDec.setSwapBytes(true);
  delay(2000);

}
else  if( startlogo==1)
{  clk.setColorDepth(8);
  TJpgDec.setJpgScale(1);
  TJpgDec.setSwapBytes(true);
  TJpgDec.drawJpg(0,0,chuyin1_240X320, sizeof(chuyin1_240X320)); //更换为初音动画1
  delay(2000);

}

else if( startlogo==2)
{   clk.setColorDepth(8);
  TJpgDec.setJpgScale(1);
  TJpgDec.setSwapBytes(true);
  TJpgDec.drawJpg(0,0,chuyin2_240X320, sizeof(chuyin2_240X320)); //更换为初音动画2
  delay(2000);

}


else if( startlogo==3)
{ 
  clk.setColorDepth(8);
  TJpgDec.setJpgScale(1);
  TJpgDec.setSwapBytes(true);
  TJpgDec.drawJpg(0,0,chuyin3_240X320, sizeof(chuyin3_240X320)); //更换为初音动画3
  delay(2000);

}




  Serial.print("AutoBright: ");
  Serial.println(AutoBright);
  Serial.print("AlarmFlag: ");
  Serial.println(AlarmFlag);  
  Serial.print("startlogo: ");
  Serial.println(startlogo);
  Serial.print("UID: ");
  Serial.println(UID);  
  Serial.print("MiniBright: ");
  Serial.println(MiniBright);  
  Serial.print("pagenum: ");
  Serial.println(pagenum); 
  Serial.print("Englishfreshtime: ");
  Serial.println(Englishfreshtime); 
  Serial.print("dbgif: ");
  Serial.println(dbgif); 
  Serial.print("waternum: ");
  Serial.println(waternum);  
  Serial.println("beforetime: "+String(hour())+"时"+String(minute())+"分"+String(second())+"秒");

  

  Serial.print("本地IP： ");
  Serial.println(WiFi.localIP());
  Serial.println("启动UDP");
  Udp.begin(localPort);
  Serial.println("等待同步...");
  setSyncProvider(getNtpTime);
  setSyncInterval(300);

  Serial.println("aftertime: "+String(hour())+"时"+String(minute())+"分"+String(second())+"秒");

  
  
  TJpgDec.setJpgScale(1);
  TJpgDec.setSwapBytes(true);
  TJpgDec.setCallback(tft_output);
  
 
 

   
  tft.fillScreen(TFT_BLACK);//清屏



  if(pagenum==1)
          {
                TJpgDec.drawJpg(15,183,temperature, sizeof(temperature));  //温度图标
                TJpgDec.drawJpg(15,213,humidity, sizeof(humidity));  //湿度图标
              
               
              
                //  获取b站粉丝数量
                get_Bstation_follow();
                fanspush();
              
                if(cityCode.length() >= 8) {
                  Serial.println("手动设置cityCode");
                  getCityWeater(); //获取天气数据
                }
                else {
                  Serial.println("自动设置cityCode");
                  getCityCode();  //获取城市代码
                }
   
           }
 
  else if(pagenum==2)
   {
     tft.setRotation(3); 
     
   }

     else if(pagenum==3)
   {

                tft.fillScreen(TFT_BLACK);
                
                tft.setTextColor(TFT_WHITE, TFT_BLACK);  // Adding a background colour erases previous text automatically
                
                // Draw clock face
                tft.fillCircle(120, 120, 118, TFT_ORANGE);
                tft.fillCircle(120, 120, 110, TFT_BLACK);
              
                // Draw 12 lines
                for(int i = 0; i<360; i+= 30) {
                  sx = cos((i-90)*0.0174532925);
                  sy = sin((i-90)*0.0174532925);
                  x0 = sx*114+120;
                  yy0 = sy*114+120;
                  x1 = sx*100+120;
                  yy1 = sy*100+120;
              
                  tft.drawLine(x0, yy0, x1, yy1, TFT_BLUE);
                }
              
                // Draw 60 dots
                for(int i = 0; i<360; i+= 6) {
                  sx = cos((i-90)*0.0174532925);
                  sy = sin((i-90)*0.0174532925);
                  x0 = sx*102+120;
                  yy0 = sy*102+120;
                  // Draw minute markers
                  tft.drawPixel(x0, yy0, TFT_WHITE);
                  
                  // Draw main quadrant dots
                  if(i==0 || i==180) tft.fillCircle(x0, yy0, 2, TFT_WHITE);
                  if(i==90 || i==270) tft.fillCircle(x0, yy0, 2, TFT_WHITE);
                  if(i==0)
                  {
                     tft.drawCentreString("12",x0,yy0+5,2);
                  }
                 else if(i==30)
                  {
                     tft.drawCentreString("1",x0-5,yy0,2);
                  }
                 else if(i==60)
                  {
                     tft.drawCentreString("2",x0-10,yy0-5,2);
                  }
                 else if(i==90)
                  {
                     tft.drawCentreString("3",x0-10,yy0-8,2);
                  }
                 else if(i==120)
                  {
                     tft.drawCentreString("4",x0-10,yy0-10,2);
                  }
                 else if(i==150)
                  {
                     tft.drawCentreString("5",x0-5,yy0-15,2);
                  }
                 else if(i==180)
                  {
                     tft.drawCentreString("6",x0,yy0-20,2);
                  }
                 else if(i==210)
                  {
                     tft.drawCentreString("7",x0+5,yy0-15,2);
                  }
                 else if(i==240)
                  {
                     tft.drawCentreString("8",x0+10,yy0-10,2);
                  }
                   else if(i==270)
                  {
                     tft.drawCentreString("9",x0+10,yy0-8,2);
                  }
                 else if(i==300)
                  {
                     tft.drawCentreString("10",x0+10,yy0-5,2);
                  }
                 else if(i==330)
                  {
                     tft.drawCentreString("11",x0+10,yy0,2);
                  }
              
                  
                }
              
                tft.fillCircle(120, 121, 3, TFT_WHITE);
              
                // Draw text at position 120,260 using fonts 4
                // Only font numbers 2,4,6,7 are valid. Font 6 only contains characters [space] 0 1 2 3 4 5 6 7 8 9 : . - a p m
                // Font 7 is a 7 segment font and only contains characters [space] 0 1 2 3 4 5 6 7 8 9 : .
               
               if(hh<=12)
               {
                  tft.drawCentreString("AM",120,260,4);
                }
              
               else if(hh>12)
               {
                 tft.drawCentreString("PM",120,260,4);
               }
              
                
              
                targetTimey = millis() + 1000; 
                   
      }
        else if(pagenum==4)
        {

                tft.initDMA();
                tft.setRotation(3);//横屏
                tft.fillScreen(TFT_BLACK);//黑色
                tft.setTextColor(TFT_WHITE,TFT_BLACK);
                
              
                if (WiFi.status() == WL_CONNECTED) //判断如果wifi连接成功
                { 

                 
                  //client.setNoDelay(false);//关闭Nagle算法
                  Serial.println("wifi is connected!");
                  Serial.print("SSID: ");
                  Serial.println(WiFi.SSID());
                  IPAddress ip = WiFi.localIP();
                  Serial.print("IP Address: ");
                  Serial.println(ip);
                  Serial.println("Port: "+String(httpPort));
                  tft.fillScreen(TFT_BLACK);//黑色
                  tft.setTextColor(TFT_WHITE,TFT_BLACK);
                  tft.drawString("Wifi Have Connected To "+String(WiFi.SSID()),20,20,2);
              
                  tft.drawString("IP: "+ip.toString(),20,40,2);
                  tft.drawString("Port: "+String(httpPort),20,60,2);
                  Serial.println("Waiting a client to connect.........");
                  servertpzs.begin(httpPort); //服务器启动监听端口号
                  servertpzs.setNoDelay(true);

                    clk.loadFont(ZdyLwFont_20);
                    clk.createSprite(320,22); 
                    clk.fillSprite(0x0000); 
                    clk.setTextDatum(CC_DATUM);
                    clk.setTextColor(TFT_WHITE, 0x0000);
                    clk.drawString("投屏助手",160,11);
                    clk.pushSprite(0,160);
                    clk.deleteSprite();
                    clk.unloadFont(); //释放加载字体资源
                }
                  TJpgDec.setJpgScale(1);
                  TJpgDec.setSwapBytes(true);
                  TJpgDec.setCallback(tft_output);//解码成功回调函数          
                   
        }


          else if(pagenum==5)
       {         
             
                  clk.loadFont(ZdyLwFont_20);
                  clk.createSprite(240,30); 
                  clk.fillSprite(0x0000); 
                  clk.setTextDatum(ML_DATUM);
                  clk.setTextColor(TFT_WHITE, 0x0000);
                  clk.drawString("英文:",5,15);
                  clk.pushSprite(0,10);
                  clk.deleteSprite();
                  clk.unloadFont(); //释放加载字体资源
              
                  clk.loadFont(ZdyLwFont_20);
                  clk.createSprite(240,30);
                  clk.fillSprite(0x0000); 
                  clk.setTextDatum(ML_DATUM);
                  clk.setTextColor(TFT_WHITE, 0x0000);
                  clk.drawString(Englishlist[3455],10,15);
                  clk.pushSprite(0,50);
                  clk.deleteSprite();
                  clk.unloadFont(); //释放加载字体资源
              
                  clk.loadFont(ZdyLwFont_20);
                  clk.createSprite(240,30); 
                  clk.fillSprite(0x0000); 
                  clk.setTextDatum(ML_DATUM);
                  clk.setTextColor(TFT_WHITE, 0x0000);
                  clk.drawString("翻译:",5,15);
                  clk.pushSprite(0,100);
                  clk.deleteSprite();
                  clk.unloadFont(); //释放加载字体资源
              
                  clk.loadFont(ZdyLwFont_20);
                  clk.createSprite(240,30);
                  clk.fillSprite(0x0000);  
                  clk.setTextDatum(ML_DATUM);
                  clk.setTextColor(TFT_WHITE, 0x0000);
                  clk.drawString(Englishfy[3455],10,15);
                  clk.pushSprite(0,140);
                  clk.deleteSprite();
                  clk.unloadFont(); //释放加载字体资源
                  smallClock();


       }

         else if(pagenum==6)
     {

        watershow();
        smallClock();
        
     
      }


      
         else if(pagenum==7)
     {
        tft.setRotation(3); 
         
        initiateGame();
        
    
     
      }


            else if(pagenum==8)
     {

        showespsys();
        
        
     
      }


      
            else if(pagenum==9)
     {

        showgy();
        
        
     
      }


      
}
time_t prevDisplay = 0; // 显示时间
unsigned long weaterTime = 0;

uint16_t read_count=0;//读取buff的长度
uint8_t pack_size[2];//用来装包大小字节
uint16_t frame_size;//当前帧大小
float start_time,end_time;//帧处理开始和结束时间
float receive_time,deal_time;//帧接收和解码时间





void loop()
{
  if(pagenum==1)

  {
           if (now() != prevDisplay) {
            prevDisplay = now();
            digitalClockDisplay();
            prevTime=0;  
          }
          
          if(millis() - weaterTime > 300000){ //5分钟更新一次天气
            weaterTime = millis();
            getCityWeater();
            get_Bstation_follow();
            fanspush();
          }
          scrollBanner();
          imgAnim();
          bilishow();

           if(digitalRead(Button) == HIGH)
          {
            delay(500);
            get_Bstation_follow();
            fanspush();
            delay(500);
            getNtpTime();

          }
  }


   if(pagenum==2)

  {
           if (now() != prevDisplay) {
            prevDisplay = now();
            FClock();
            prevTime=0;  
          }

           if(digitalRead(Button) == HIGH)
          {
            delay(500);
            getNtpTime();
          }
          
  }


  if(pagenum==3)

  {
             if (targetTimey < millis()) {
              targetTimey += 1000;
              ss++;              // Advance second
              if (ss==60) {
                ss=0;
                mm++;            // Advance minute
                if(mm>59) {
                  mm=0;
                  hh++;          // Advance hour
                  if (hh>23) {
                    hh=0;
                  }
                }
              }
          
              // Pre-compute hand degrees, x & y coords for a fast screen update
              sdeg = ss*6;                  // 0-59 -> 0-354
              mdeg = mm*6+sdeg*0.01666667;  // 0-59 -> 0-360 - includes seconds
              hdeg = hh*30+mdeg*0.0833333;  // 0-11 -> 0-360 - includes minutes and seconds
              hx = cos((hdeg-90)*0.0174532925);    
              hy = sin((hdeg-90)*0.0174532925);
              mx = cos((mdeg-90)*0.0174532925);    
              my = sin((mdeg-90)*0.0174532925);
              sx = cos((sdeg-90)*0.0174532925);    
              sy = sin((sdeg-90)*0.0174532925);
          
              if (ss==0 || initial) {
                initial = 0;
                // Erase hour and minute hand positions every minute
                tft.drawLine(ohx, ohy, 120, 121, TFT_BLACK);
                ohx = hx*62+121;    
                ohy = hy*62+121;
                tft.drawLine(omx, omy, 120, 121, TFT_BLACK);
                omx = mx*84+120;    
                omy = my*84+121;
              }
          
                // Redraw new hand positions, hour and minute hands not erased here to avoid flicker
                tft.drawLine(osx, osy, 120, 121, TFT_BLACK);
                osx = sx*90+121;    
                osy = sy*90+121;
                tft.drawLine(osx, osy, 120, 121, TFT_RED);
                tft.drawLine(ohx, ohy, 120, 121, TFT_WHITE);
                tft.drawLine(omx, omy, 120, 121, TFT_WHITE);
                tft.drawLine(osx, osy, 120, 121, TFT_RED);
          
              tft.fillCircle(120, 121, 3, TFT_RED);
            }

          if(digitalRead(Button) == HIGH)
          {
            delay(500);
            getNtpTime();
          }
          

      
          
  }

   if(pagenum==4)

  {
             // put your main code here, to run repeatedly:
            //沾包问题 recv阻塞，长时间收不到数据就会断开
            //断开连接原因，读取buff太快，上位机发送太快造成buff溢出，清空缓冲区会断开（FLUSH）
            WiFiClient clienttpzs = servertpzs.available(); //尝试建立客户对象
            if(clienttpzs){
              Serial.println("[New Client!]");
              clienttpzs.write("ok");//向上位机发送下一帧发送指令
              
              while(clienttpzs.connected())//如果客户端处于连接状态client.connected()
              {
                clienttpzs.write("no");//向上位机发送当前帧未写入完指令
                while(clienttpzs.available()){
                  while (clienttpzs.available()) {//检测缓冲区是否有数据
                       if(read_count==0)
                        {
                          start_time=millis();
                          clienttpzs.read(pack_size,2);//读取帧大小
                          frame_size=pack_size[0]+(pack_size[1]<<8);
                       }
                       read_count=clienttpzs.read(buff,7000);//向缓冲区读取数据
                       memcpy(&img_buff[size_count],buff,read_count);//将读取的buff字节地址复制给img_buff数组
                       size_count=size_count+read_count;//计数当前帧字节位置
            //           Serial.println(size_count);
                       if(img_buff[frame_size-3]==0xaa && img_buff[frame_size-2]==0xbb && img_buff[frame_size-1]==0xcc)//判断末尾数据是否当前帧校验位
                       {
                        receive_time=millis()-start_time;
                        deal_time=millis();
                        img_buff[frame_size-3]=0;img_buff[frame_size-2]=0;img_buff[frame_size-1]=0;//清除标志位
                        tft.startWrite();//必须先使用startWrite，以便TFT芯片选择保持低的DMA和SPI通道设置保持配置
                        TJpgDec.drawJpg(0,0,img_buff, sizeof(img_buff));//在左上角的0,0处绘制图像——在这个草图中，DMA请求在回调tft_output()中处理
                        tft.endWrite();//必须使用endWrite来释放TFT芯片选择和释放SPI通道吗
            //            memset(&img_buff,0,sizeof(img_buff));//清空buff
                        size_count=0;//下一帧
                        read_count=0;
                        clienttpzs.write("ok");//向上位机发送下一帧发送指令
                        end_time = millis(); //计算mcu刷新一张图片的时间，从而算出1s能刷新多少张图，即得出最大刷新率
                        Serial.printf("帧大小：%d " ,frame_size);Serial.print("MCU处理速度："); Serial.print(1000 / (end_time - start_time), 2); Serial.print("Fps");
                        Serial.printf("帧接收耗时:%.2fms,帧解码显示耗时:%.2fms\n",receive_time,(millis()-deal_time));
                        break;
                        }       
                  }
                }
              }
              clienttpzs.stop();
              Serial.println("连接中断,请复位重新创建服务端");
            }
  }

       if(pagenum==5)

   {
     
           if (now() != prevDisplay)
            {
            prevDisplay = now();
            smallClock();
            prevTime=0;  
            }        

           if(millis() - weaterTime > Truefreshtime){ //默认1分钟更新一次单词本
           weaterTime = millis();
           showEnglish();
          }

          if(digitalRead(Button) == HIGH)
          {  
             delay(500);
             showEnglish();
             delay(500);
             getNtpTime();
          }
         
   }


         if(pagenum==6)

   {
           clearwaternum();
           if (now() != prevDisplay)
            {
            prevDisplay = now();
            smallClock();
            prevTime=0;  
            }        
            
          
          if(digitalRead(Button) == HIGH)
          {  
             delay(500);
             waternum=waternum+1;
             EEPROM.write(61,waternum);
             EEPROM.commit(); 
             watershow();           
             getNtpTime();
          }
         
   }



          if(pagenum==7)

   {
                 
                xP=xP-movingRate; // xP - x coordinate of the pilars; range: 319 - (-51)   
                drawPilars(xP, yP); // Draws the pillars 
                
                // yB - y coordinate of the bird which depends on value of the fallingRate variable
                yB+=fallRateInt; 
                fallRate=fallRate+0.4; // Each inetration the fall rate increase so that we can the effect of acceleration/ gravity
                fallRateInt= int(fallRate);
                
                // Checks for collision
                if(yB>=180 || yB<=0){ // top and bottom
                  gameOver();
                }
                if((xP<=85) && (xP>=5) && (yB<=yP-2)){ // upper pillar
                  gameOver();
                }
                if((xP<=85) && (xP>=5) && (yB>=yP+60)){ // lower pillar
                  gameOver();
                }
                
                // Draws the bird
                drawBird(yB);
                 
                // After the pillar has passed through the screen
                if (xP<=-51){
                  xP=319; // Resets xP to 319
                  yP = rand() % 100+20; // Random number for the pillars height
                  score++; // Increase score by one
                }
                //==== Controlling the bird
            //    if (myTouch.dataAvailable()&& !screenPressed) {
            //       fallRate=-6; // Setting the fallRate negative will make the bird jump
            //       screenPressed = true;
            //   }
                // Doesn't allow holding the screen / you must tap it
            //    else if ( !myTouch.dataAvailable() && screenPressed){
            //    screenPressed = false;
            //    }
               
                 if(digitalRead(Button) == HIGH&& !screenPressed)
                 {   
                   
                     delay(10);//需要延时防抖动
                     fallRate=-6;
                     screenPressed = true;
                     
                 }
            
               
                else if(digitalRead(Button) != HIGH&& screenPressed)//防止长按按键           
                {  
                  delay(10);//需要延时防抖动
                  screenPressed = false;
                  
                }
            
              
            //每隔5分加速一次
                
                // After each five points, increases the moving rate of the pillars
                if((score - lastSpeedUpScore)==5)
                {
                  lastSpeedUpScore=score;
                  movingRate++;
                }

          
          
      
   }





    if(pagenum==8)
    {
      if(digitalRead(Button) == HIGH)
      {
        delay(100);
        showespsys();
      }

    }


     else   if(pagenum==9)
    {
      showgy();
      

    }
     
  
  Serial_set();
  switch(AutoBright) 
     { //屏幕背光控制
    case true:Filter_Value = Filter();ledcWrite(0,map(Filter_Value,MiniBright,4095,MiniBright,255));break;
    case false:ledcWrite(0,LCD_BL_PWM);break;
     }

 sysnzmode();
 
}


// 发送HTTP请求并且将服务器响应通过串口输出
void getCityCode(){
 String URL = "http://wgeo.weather.com.cn/ip/?_="+String(now());
  //创建 HTTPClient 对象
  HTTPClient httpClient;
 
  //配置请求地址。此处也可以不使用端口号和PATH而单纯的
  httpClient.begin(wificlient,URL); 
  
  //设置请求头中的User-Agent
  httpClient.setUserAgent("Mozilla/5.0 (iPhone; CPU iPhone OS 11_0 like Mac OS X) AppleWebKit/604.1.38 (KHTML, like Gecko) Version/11.0 Mobile/15A372 Safari/604.1");
  httpClient.addHeader("Referer", "http://www.weather.com.cn/");
 
  //启动连接并发送HTTP请求
  int httpCode = httpClient.GET();
  Serial.print("Send GET request to URL: ");
  Serial.println(URL);
  
  //如果服务器响应OK则从服务器获取响应体信息并通过串口输出
  if (httpCode == HTTP_CODE_OK) {
    String str = httpClient.getString();
    
    int aa = str.indexOf("id=");
    if(aa>-1)
    {
       //cityCode = str.substring(aa+4,aa+4+9).toInt();
       cityCode = str.substring(aa+4,aa+4+9);
       Serial.println(cityCode); 
       getCityWeater();
    }
    else
    {
      Serial.println("获取城市代码失败");  
    }
    
    
  } else {
    Serial.println("请求城市代码错误：");
    Serial.println(httpCode);
  }
 
  //关闭ESP32与服务器连接
  httpClient.end();
}



// 获取城市天气
void getCityWeater(){
 //String URL = "http://d1.weather.com.cn/dingzhi/" + cityCode + ".html?_="+String(now());//新
 String URL = "http://d1.weather.com.cn/weather_index/" + cityCode + ".html?_="+String(now());//原来
  //创建 HTTPClient 对象
  HTTPClient httpClient;
  
  httpClient.begin(URL); 
  
  //设置请求头中的User-Agent
  httpClient.setUserAgent("Mozilla/5.0 (iPhone; CPU iPhone OS 11_0 like Mac OS X) AppleWebKit/604.1.38 (KHTML, like Gecko) Version/11.0 Mobile/15A372 Safari/604.1");
  httpClient.addHeader("Referer", "http://www.weather.com.cn/");
 
  //启动连接并发送HTTP请求
  int httpCode = httpClient.GET();
  Serial.println("正在获取天气数据");
  Serial.println(URL);
  
  //如果服务器响应OK则从服务器获取响应体信息并通过串口输出
  if (httpCode == HTTP_CODE_OK) {

    String str = httpClient.getString();
    int indexStart = str.indexOf("weatherinfo\":");
    int indexEnd = str.indexOf("};var alarmDZ");

    String jsonCityDZ = str.substring(indexStart+13,indexEnd);
    //Serial.println(jsonCityDZ);

    indexStart = str.indexOf("dataSK =");
    indexEnd = str.indexOf(";var dataZS");
    String jsonDataSK = str.substring(indexStart+8,indexEnd);
    //Serial.println(jsonDataSK);

    
    indexStart = str.indexOf("\"f\":[");
    indexEnd = str.indexOf(",{\"fa");
    String jsonFC = str.substring(indexStart+5,indexEnd);
    //Serial.println(jsonFC);
    
    weaterData(&jsonCityDZ,&jsonDataSK,&jsonFC);
    Serial.println("获取成功");
    
  } else {
    Serial.println("请求城市天气错误：");
    Serial.print(httpCode);
  }
 
  //关闭ESP32与服务器连接
  httpClient.end();
}


String scrollText[7];
//int scrollTextWidth = 0;
//天气信息写到屏幕上
void weaterData(String *cityDZ,String *dataSK,String *dataFC)
{
  //解析第一段JSON
  DynamicJsonDocument doc(1024);
  deserializeJson(doc, *dataSK);
  JsonObject sk = doc.as<JsonObject>();

  //TFT_eSprite clkb = TFT_eSprite(&tft);
  
  /***绘制相关文字***/
  clk.setColorDepth(8);
  clk.loadFont(ZdyLwFont_20);
  
  //温度
  clk.createSprite(58, 24); 
  clk.fillSprite(bgColor);
  clk.setTextDatum(CC_DATUM);
  clk.setTextColor(TFT_WHITE, bgColor); 
  clk.drawString(sk["temp"].as<String>()+"℃",28,13);
  clk.pushSprite(100,184);
  clk.deleteSprite();
  tempnum = sk["temp"].as<int>();
  tempnum = tempnum+10;
  if(tempnum<10)
    tempcol=0x00FF;
  else if(tempnum<28)
    tempcol=0x0AFF;
  else if(tempnum<34)
    tempcol=0x0F0F;
  else if(tempnum<41)
    tempcol=0xFF0F;
  else if(tempnum<49)
    tempcol=0xF00F;
  else
  {
    tempcol=0xF00F;
    tempnum=50;
  }
  tempWin();
  
  //湿度
  clk.createSprite(58, 24); 
  clk.fillSprite(bgColor);
  clk.setTextDatum(CC_DATUM);
  clk.setTextColor(TFT_WHITE, bgColor); 
  clk.drawString(sk["SD"].as<String>(),28,13);
  //clk.drawString("100%",28,13);
  clk.pushSprite(100,214);
  clk.deleteSprite();
  //String A = sk["SD"].as<String>();
  huminum = atoi((sk["SD"].as<String>()).substring(0,2).c_str());
  
  if(huminum>90)
    humicol=0x00FF;
  else if(huminum>70)
    humicol=0x0AFF;
  else if(huminum>40)
    humicol=0x0F0F;
  else if(huminum>20)
    humicol=0xFF0F;
  else
    humicol=0xF00F;
  humidityWin();

  
  //城市名称
  clk.createSprite(94, 30); 
  clk.fillSprite(bgColor);
  clk.setTextDatum(CC_DATUM);
  clk.setTextColor(TFT_WHITE, bgColor); 
  clk.drawString(sk["cityname"].as<String>(),44,16);
  clk.pushSprite(15,15);
  clk.deleteSprite();

  //PM2.5空气指数
  uint16_t pm25BgColor = tft.color565(156,202,127);//优
  String aqiTxt = "优";
  int pm25V = sk["aqi"];
  if(pm25V>200){
    pm25BgColor = tft.color565(136,11,32);//重度
    aqiTxt = "重度";
  }else if(pm25V>150){
    pm25BgColor = tft.color565(186,55,121);//中度
    aqiTxt = "中度";
  }else if(pm25V>100){
    pm25BgColor = tft.color565(242,159,57);//轻
    aqiTxt = "轻度";
  }else if(pm25V>50){
    pm25BgColor = tft.color565(247,219,100);//良
    aqiTxt = "良";
  }
  clk.createSprite(56, 24); 
  clk.fillSprite(bgColor);
  clk.fillRoundRect(0,0,50,24,4,pm25BgColor);
  clk.setTextDatum(CC_DATUM);
  clk.setTextColor(0x0000); 
  clk.drawString(aqiTxt,25,13);
  clk.pushSprite(104,18);
  clk.deleteSprite();
  
  scrollText[0] = "实时天气 "+sk["weather"].as<String>();
  scrollText[1] = "空气质量 "+aqiTxt;
  scrollText[2] = "风向 "+sk["WD"].as<String>()+sk["WS"].as<String>();

  //scrollText[6] = atoi((sk["weathercode"].as<String>()).substring(1,3).c_str()) ;

  //天气图标
  wrat.printfweather(170,15,atoi((sk["weathercode"].as<String>()).substring(1,3).c_str()));

  
  //左上角滚动字幕
  //解析第二段JSON
  deserializeJson(doc, *cityDZ);
  JsonObject dz = doc.as<JsonObject>();
  //Serial.println(sk["ws"].as<String>());
  //横向滚动方式
  //String aa = "今日天气:" + dz["weather"].as<String>() + "，温度:最低" + dz["tempn"].as<String>() + "，最高" + dz["temp"].as<String>() + " 空气质量:" + aqiTxt + "，风向:" + dz["wd"].as<String>() + dz["ws"].as<String>();
  //scrollTextWidth = clk.textWidth(scrollText);
  //Serial.println(aa);
  scrollText[3] = "今日"+dz["weather"].as<String>();
  
  deserializeJson(doc, *dataFC);
  JsonObject fc = doc.as<JsonObject>();
  
  scrollText[4] = "最低温度"+fc["fd"].as<String>()+"℃";
  scrollText[5] = "最高温度"+fc["fc"].as<String>()+"℃";
  
  //Serial.println(scrollText[0]);
  
  clk.unloadFont();
}

int currentIndex = 0;
TFT_eSprite clkb = TFT_eSprite(&tft);

void scrollBanner(){
  //if(millis() - prevTime > 2333) //3秒切换一次
  if(second()%2 ==0&& prevTime == 0)
  { 
    if(scrollText[currentIndex])
    {
      clkb.setColorDepth(8);
      clkb.loadFont(ZdyLwFont_20);
      clkb.createSprite(150, 30); 
      clkb.fillSprite(bgColor);
      clkb.setTextWrap(false);
      clkb.setTextDatum(CC_DATUM);
      clkb.setTextColor(TFT_WHITE, bgColor); 
      clkb.drawString(scrollText[currentIndex],74, 16);
      clkb.pushSprite(10,45);
       
      clkb.deleteSprite();
      clkb.unloadFont();
      
      if(currentIndex>=5)
        currentIndex = 0;  //回第一个
      else
        currentIndex += 1;  //准备切换到下一个        
    }
    prevTime = 1;
  }
}


void imgAnim()
{
  int x=160,y=160;
  if(millis() - AprevTime > 37) //x ms切换一次
  {
    Anim++;
    AprevTime = millis();
  }
  if(Anim==10)
    Anim=0;

  switch(Anim)
  {
    case 0:
      TJpgDec.drawJpg(x,y,i0, sizeof(i0));
      break;
    case 1:
      TJpgDec.drawJpg(x,y,i1, sizeof(i1));
      break;
    case 2:
      TJpgDec.drawJpg(x,y,i2, sizeof(i2));
      break;
    case 3:
      TJpgDec.drawJpg(x,y,i3, sizeof(i3));
      break;
    case 4:
      TJpgDec.drawJpg(x,y,i4, sizeof(i4));
      break;
    case 5:
      TJpgDec.drawJpg(x,y,i5, sizeof(i5));
      break;
    case 6:
      TJpgDec.drawJpg(x,y,i6, sizeof(i6));
      break;
    case 7:
      TJpgDec.drawJpg(x,y,i7, sizeof(i7));
      break;
    case 8: 
      TJpgDec.drawJpg(x,y,i8, sizeof(i8));
      break;
    case 9: 
      TJpgDec.drawJpg(x,y,i9, sizeof(i9));
      break;
    default:
      Serial.println("显示Anim错误");
      break;
  }
}

unsigned char Hour_sign   = 60;
unsigned char Minute_sign = 60;
unsigned char Second_sign = 60;
void digitalClockDisplay()
{ 
  int timey=82;
  if(hour()!=Hour_sign)//时钟刷新
  {
    dig.printfW3660(20,timey,hour()/10);
    dig.printfW3660(60,timey,hour()%10);
    Hour_sign = hour();
  }
  if(minute()!=Minute_sign)//分钟刷新
  {
    dig.printfO3660(101,timey,minute()/10);
    dig.printfO3660(141,timey,minute()%10);
    Minute_sign = minute();
  }
  if(second()!=Second_sign)//分钟刷新
  {
    dig.printfW1830(182,timey+30,second()/10);
    dig.printfW1830(202,timey+30,second()%10);
    Second_sign = second();
  }
  
  /***日期****/
  clk.setColorDepth(8);
  clk.loadFont(ZdyLwFont_20);
  
  //星期
  clk.createSprite(58, 30);
  clk.fillSprite(bgColor);
  clk.setTextDatum(CC_DATUM);
  clk.setTextColor(TFT_WHITE, bgColor);
  clk.drawString(week(),29,16);
  clk.pushSprite(102,150);
  clk.deleteSprite();
  
  //月日
  clk.createSprite(95, 30);
  clk.fillSprite(bgColor);
  clk.setTextDatum(CC_DATUM);
  clk.setTextColor(TFT_WHITE, bgColor);  
  clk.drawString(monthDay(),49,16);
  clk.pushSprite(5,150);
  clk.deleteSprite();
  
  clk.unloadFont();
  /***日期****/
}

//星期
String week()
{
  String wk[7] = {"日","一","二","三","四","五","六"};
  String s = "周" + wk[weekday()-1];
  return s;
}

//月日
String monthDay()
{
  String s = String(month());
  s = s + "月" + day() + "日";
  return s;
}


//------------------------------------------------------------------------------------------------

void FClock()
{
  
  clk.setColorDepth(8);

  /***中间时间区***/
  //时分
  clk.createSprite(215,76);
  clk.fillSprite(bgColor);
  clk.loadFont(zkyyt72);
  clk.setTextDatum(ML_DATUM);
  clk.setTextColor(TFT_GOLD, bgColor);
  clk.drawString(hourMinute(),1,38); //绘制时和分
  //clk.unloadFont();
  clk.pushSprite(0,60);
  clk.deleteSprite();
  
  //秒
  clk.createSprite(100,76);
  clk.fillSprite(bgColor);
  
  clk.loadFont(zkyyt72);
  clk.setTextDatum(ML_DATUM);
  clk.setTextColor(TFT_GOLD, bgColor); 
  clk.drawString(num2str(second()),1,38);
  
  clk.unloadFont();
  clk.pushSprite(220,60);
  clk.deleteSprite();
  /***中间时间区***/

  /***底部***/
  clk.loadFont(zkyyt48);
  clk.createSprite(100, 54);
  clk.fillSprite(bgColor);

  //星期
  clk.setTextDatum(ML_DATUM);
  clk.setTextColor(0xC618, bgColor);
  clk.drawString(fweek(),1,27);
  clk.pushSprite(220,175);
  clk.deleteSprite();
  
  //月日
  clk.createSprite(215, 54);
  clk.fillSprite(bgColor);
  clk.setTextDatum(ML_DATUM);
  clk.setTextColor(0xC618, bgColor);  
  clk.drawString(fmonthDay(),1,27);
  clk.pushSprite(1,175);
  clk.deleteSprite();
  
  clk.unloadFont();
  /***底部***/

  
}

//星期
String fweek(){
  String wk[7] = {"日","一","二","三","四","五","六"};
  String s = "周" + wk[weekday()-1];
  return s;
}

//月日
String fmonthDay(){
  String s = String(month());
  s = s + "月" + day() + "日";
  return s;
}
//时分
String hourMinute(){
  String s = num2str(hour());
  backLight_hour = s.toInt();
  s = s + ":" + num2str(minute());
  return s;
}

String num2str(int digits)
{
  String s = "";
  if (digits < 10)
    s = s + "0";
  s = s + digits;
  return s;
}

void printDigits(int digits)
{
  Serial.print(":");
  if (digits < 10)
    Serial.print('0');
  Serial.print(digits);
}

//小型时钟界面
void smallClock()
{
  
  clk.setColorDepth(8);

  /***中间时间区***/
  //时分
  clk.createSprite(60,30);
  clk.fillSprite(0x0000);
  clk.loadFont(ZdyLwFont_20);
  clk.setTextDatum(ML_DATUM);
  clk.setTextColor(TFT_SKYBLUE, 0x0000);
  clk.drawString(hourMinute(),5,15); //绘制时和分
  clk.unloadFont();
  clk.pushSprite(152,280);
  clk.deleteSprite();
  
  //秒
  clk.createSprite(26,30);
  clk.fillSprite(0x0000);
  
  clk.loadFont(ZdyLwFont_20);
  clk.setTextDatum(ML_DATUM);
  clk.setTextColor(TFT_SKYBLUE, 0x0000); 
  clk.drawString(num2str(second()),1,15);
  
  clk.unloadFont();
  clk.pushSprite(213,280);
  clk.deleteSprite();

   /***底部***/
  clk.loadFont(ZdyLwFont_20);
  clk.createSprite(50,30);
  clk.fillSprite(0x0000);

  //星期
  clk.setTextDatum(ML_DATUM);
  clk.setTextColor(TFT_SKYBLUE, 0x0000);
  clk.drawString(fweek(),5,15);
  clk.pushSprite(101,280);
  clk.deleteSprite();
  
  //月日
  clk.createSprite(100, 30);
  clk.fillSprite(0x0000);
  clk.setTextDatum(ML_DATUM);
  clk.setTextColor(TFT_SKYBLUE, 0x0000);  
  clk.drawString(fmonthDay(),1,15);
  clk.pushSprite(0,280);
  clk.deleteSprite();
  
  clk.unloadFont();
  /***底部***/
}



/*-------- NTP code ----------*/

const int NTP_PACKET_SIZE = 48; // NTP时间在消息的前48字节中
byte packetBuffer[NTP_PACKET_SIZE]; //buffer to hold incoming & outgoing packets

time_t getNtpTime()
{
  IPAddress ntpServerIP; // NTP server's ip address

  while (Udp.parsePacket() > 0) ; // discard any previously received packets
  //Serial.println("Transmit NTP Request");
  // get a random server from the pool
  WiFi.hostByName(ntpServerName, ntpServerIP);
  //Serial.print(ntpServerName);
  //Serial.print(": ");
  //Serial.println(ntpServerIP);
  sendNTPpacket(ntpServerIP);
  uint32_t beginWait = millis();
  while (millis() - beginWait < 1500) {
    int size = Udp.parsePacket();
    if (size >= NTP_PACKET_SIZE) {
      Serial.println("Receive NTP Response");
      Udp.read(packetBuffer, NTP_PACKET_SIZE);  // read packet into the buffer
      unsigned long secsSince1900;
      // convert four bytes starting at location 40 to a long integer
      secsSince1900 =  (unsigned long)packetBuffer[40] << 24;
      secsSince1900 |= (unsigned long)packetBuffer[41] << 16;
      secsSince1900 |= (unsigned long)packetBuffer[42] << 8;
      secsSince1900 |= (unsigned long)packetBuffer[43];
      //Serial.println(secsSince1900 - 2208988800UL + timeZone * SECS_PER_HOUR);
      return secsSince1900 - 2208988800UL + timeZone * SECS_PER_HOUR;
    }
  }
  Serial.println("No NTP Response :-(");
  return 0; // 无法获取时间时返回0
}

// 向NTP服务器发送请求
void sendNTPpacket(IPAddress &address)
{
  // set all bytes in the buffer to 0
  memset(packetBuffer, 0, NTP_PACKET_SIZE);
  // Initialize values needed to form NTP request
  // (see URL above for details on the packets)
  packetBuffer[0] = 0b11100011;   // LI, Version, Mode
  packetBuffer[1] = 0;     // Stratum, or type of clock
  packetBuffer[2] = 6;     // Polling Interval
  packetBuffer[3] = 0xEC;  // Peer Clock Precision
  // 8 bytes of zero for Root Delay & Root Dispersion
  packetBuffer[12] = 49;
  packetBuffer[13] = 0x4E;
  packetBuffer[14] = 49;
  packetBuffer[15] = 52;
  // all NTP fields have been given values, now
  // you can send a packet requesting a timestamp:
  Udp.beginPacket(address, 123); //NTP requests are to port 123
  Udp.write(packetBuffer, NTP_PACKET_SIZE);
  Udp.endPacket();
}
