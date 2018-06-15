#include "mbed.h"
#include "Hexi_KW40Z.h"
#include "Hexi_OLED_SSD1351.h"
#include "OLED_types.h"
#include "OpenSans_Font.h"
#include "string.h"

#define LED_ON      0
#define LED_OFF     1


void StartHaptic(void);
void StopHaptic(void const *n);


Serial pc(USBTX, USBRX);

DigitalOut redLed(LED1,1);
DigitalOut greenLed(LED2,1);
DigitalOut blueLed(LED3,1);
DigitalOut haptic(PTB9);

/* Define timer for haptic feedback */
RtosTimer hapticTimer(StopHaptic, osTimerOnce);

/* Instantiate the Hexi KW40Z Driver (UART TX, UART RX) */ 
KW40Z kw40z_device(PTE24, PTE25);

/* Instantiate the SSD1351 OLED Driver */ 
SSD1351 oled(PTB22,PTB21,PTC13,PTB20,PTE6, PTD15); /* (MOSI,SCLK,POWER,CS,RST,DC) */

 /* Text Buffer */ 
char text[20]; 
// All the Possible States
bool drawChallenger = false;
bool drawGame = false;
bool isChallenger = false;
bool isGame = false;
bool isDefault = false;
bool drawDefault = true;
bool isWait1 = false;
bool drawWait1 = false;
bool isWait2 = false;
bool drawWait2 = false;
bool isResult = false;
bool drawResult = false;
bool isRejection = false;
bool drawRejection = false;
//Your choice of R/P/S; R = 1, P = 2, S = 3
uint8_t choice = 0;
//Name of opponent
char challengerName[5];
//Result of challenge; 'w' for win, anything else is loss
char result;
//Send your choice in battery
void sendSignal()
{
    kw40z_device.SendSetApplicationMode(GUI_CURRENT_APP_SENSOR_TAG);
    kw40z_device.SendBatteryLevel(choice);
    Thread::wait(1000);
    pc.printf("sent: %d", choice);
}

/****************************Call Back Functions*******************************/
//Used for "No" on challenge/result; Scissors on game
void ButtonRight(void)
{
    StartHaptic();
    redLed      = LED_ON;
    greenLed    = LED_OFF;
    blueLed     = LED_OFF;
    if(isGame)
    {
        isGame = false;
        choice = 3;
        drawWait2 = true;
        sendSignal();
    }
    else if(isChallenger || isResult)
    {
        drawDefault = true;
        isChallenger = false;
        isResult = false;
        // Send rejection to Pi
        choice = 5;
        sendSignal();
    }
}
//Yes on challenge/result
void ButtonLeft(void)
{
    StartHaptic();
    redLed      = LED_OFF;
    greenLed    = LED_ON;
    blueLed     = LED_OFF;
    if(isChallenger || isResult)
    {
        drawWait1 = true;
        isChallenger = false;
        isResult = false;
        //Send accept to Pi
        choice = 4;
        sendSignal();
    }
}
//Rock on game
void ButtonUp(void)
{
    StartHaptic();
    
    redLed      = LED_OFF;
    greenLed    = LED_OFF;
    blueLed     = LED_ON;
    if(isGame)
    {
        isGame = false;
        choice = 1;
        drawWait2 = true;
        sendSignal();
    }
}
//Scissors on Game
void ButtonDown(void)
{
    StartHaptic();
    
    redLed      = LED_ON;
    greenLed    = LED_ON;
    blueLed     = LED_OFF;
    if(isGame)
    {
        isGame = false;
        choice = 2;
        drawWait2 = true;
        sendSignal();
    }
}
//Input from pi
void AlertReceived(uint8_t *data, uint8_t length)
{
    StartHaptic();
    data[10] = 0;
    pc.printf("%s\n\r", data);
    //Is name of challenger in Default
    if(isDefault)
    {
        isDefault = false;
        for(int i = 0; i < 4; i++)
            challengerName[i] = data[i];
        challengerName[4] = '\0';
        drawChallenger = true;
    }
    //Is 'a' for accept or anything else for reject
    else if(isWait1)
    {
        isWait1 = false;
        if(data[0] == 'a')
            drawGame = true;
        else
            drawRejection = true;
    }
    //Pi sends back result of game
    else if(isWait2)
    {
        isWait2 = false;
        result = data[0];
        drawResult = true;
    }
}
//Send your name to pi
void SendName()
{
    //temp used to identify a game playing JKG
    uint16_t temp = 1007;
    //humidity and pressure make up the name
    uint16_t humidity = ((uint16_t)'B'<< 8) + 'r';
    uint16_t pressure = ((uint16_t)'y' << 8) + 'n';
    kw40z_device.SendTemperature(temp);
    kw40z_device.SendHumidity(humidity);
    kw40z_device.SendPressure(pressure);
}
oled_text_properties_t textProperties = {0};
/***********************End of Call Back Functions*****************************/

/********************Screen Drawing Functions**********************************/

void newChallenger()
{
    drawChallenger = false;
    isChallenger = true;
    oled.FillScreen(COLOR_BLACK);
        
    /* Change font color to Blue */ 
    textProperties.fontColor   = COLOR_YELLOW;
    textProperties.alignParam = OLED_TEXT_ALIGN_LEFT;
    oled.SetTextProperties(&textProperties);
    
    /* Display Bluetooth Label at x=35,y=10 */ 
    oled.Label((uint8_t *)challengerName,35,10);
    
    textProperties.fontColor   = COLOR_WHITE;

    oled.SetTextProperties(&textProperties);
    
    /* Display Bluetooth Label at x=5,y=25 */ 
    strcpy((char *) text,"challenges you!");
    oled.Label((uint8_t *)text,5,25);
    

    
    /* Display Bluetooth Label at x=18,y=40 */ 
    strcpy((char *) text,"Accept the");
    oled.Label((uint8_t *)text,18,40);
    
    /* Display Bluetooth Label at x=5,y=65 */ 
    strcpy((char *) text,"challenge?");
    oled.Label((uint8_t *)text,20,55);
    
    /* Change font color to white */ 
    textProperties.fontColor   = COLOR_YELLOW;
    oled.SetTextProperties(&textProperties);
    
    /* Display Label at x=22,y=80 */ 
    strcpy((char *) text,"Yes/No");
    oled.Label((uint8_t *)text,28,80);
}

void newGame()
{
    drawGame = false;
    isGame = true;
    oled.FillScreen(COLOR_BLACK);
        
    /* Change font color to Blue */ 
    textProperties.fontColor   = COLOR_BLUE;
    textProperties.alignParam = OLED_TEXT_ALIGN_RIGHT;
    oled.SetTextProperties(&textProperties);
    
    /* Display Bluetooth Label at x=5,y=10 */ 
    strcpy((char *) text,"Choose a move!");
    oled.Label((uint8_t *)text,3,0);
    
    textProperties.fontColor   = COLOR_BLUE;
    oled.SetTextProperties(&textProperties);
    
    /* Display Bluetooth Label at x=60,y=25 */ 
    strcpy((char *) text,"Rock");
    oled.Label((uint8_t *)text,70,15);
    
    textProperties.fontColor   = COLOR_YELLOW;
    oled.SetTextProperties(&textProperties);
    
    /* Display Bluetooth Label at x=55,y=50 */ 
    strcpy((char *) text,"Paper");
    oled.Label((uint8_t *)text,65,50);
    
    textProperties.fontColor   = COLOR_RED;
    oled.SetTextProperties(&textProperties);
    
    /* Display Bluetooth Label at x=50,y=75 */ 
    strcpy((char *) text,"Scissors");
    oled.Label((uint8_t *)text,50,80);
    
}
void newRejection()
{
    drawRejection = false;
    isRejection = true;
    oled.FillScreen(COLOR_BLACK);
    textProperties.fontColor   = COLOR_RED;
    oled.SetTextProperties(&textProperties);
    strcpy((char *) text,"Challenge");
    oled.Label((uint8_t *)text,20,30);
    strcpy((char *) text,"Rejected");
    oled.Label((uint8_t *)text,23,45);
}
void wait1()
{
    drawWait1 = false;
    isWait1 = true;
    oled.FillScreen(COLOR_BLACK);
    textProperties.fontColor   = COLOR_BLUE;
    oled.SetTextProperties(&textProperties);
    strcpy((char *) text,"Waiting...");
    oled.Label((uint8_t *)text,8,38);
}
void wait2()
{
    drawWait2 = false;
    isWait2 = true;
    oled.FillScreen(COLOR_BLACK);
    textProperties.fontColor   = COLOR_BLUE;
    oled.SetTextProperties(&textProperties);
    strcpy((char *) text,"Waiting...");
    oled.Label((uint8_t *)text,8,38);
}
void newResult()
{
    drawResult = false;
    isResult = true;
    oled.FillScreen(COLOR_BLACK);
    if(result == 'w')
    {
        textProperties.fontColor   = COLOR_GREEN;
        oled.SetTextProperties(&textProperties);
        strcpy((char *) text,"VICTORY");
        oled.Label((uint8_t *)text,26,28);
    }
    else if(result == 't')
    {
        textProperties.fontColor   = COLOR_WHITE;
        oled.SetTextProperties(&textProperties);
        strcpy((char *) text,"DRAW");
        oled.Label((uint8_t *)text,26,28);
    }
    else
    {
        textProperties.fontColor   = COLOR_RED;
        oled.SetTextProperties(&textProperties);
        strcpy((char *) text,"DEFEAT");
        oled.Label((uint8_t *)text,28,28);
    }
    textProperties.fontColor   = COLOR_WHITE;
    oled.SetTextProperties(&textProperties);
    strcpy((char *) text,"Play again?");
    oled.Label((uint8_t *)text, 8, 58);
    strcpy((char *) text,"Yes/No");
    oled.Label((uint8_t *)text,28,80);
}
/**********************************MAIN****************************************/
int main()
{    
    /* Register callbacks to application functions */
    kw40z_device.attach_buttonUp(&ButtonUp);
    kw40z_device.attach_buttonDown(&ButtonDown);
    kw40z_device.attach_buttonLeft(&ButtonLeft);
    kw40z_device.attach_buttonRight(&ButtonRight);
    kw40z_device.attach_alert(&AlertReceived);
    /* Turn on the backlight of the OLED Display */
    oled.DimScreenON();
    //Always broadcast
    if(!kw40z_device.GetAdvertisementMode())
        kw40z_device.ToggleAdvertisementMode();
    /* Get OLED Class Default Text Properties */ 
    oled.GetTextProperties(&textProperties);         
    int counter = 0;
    int timeout = 0;
    while (true) 
    {
        //Send name every 2.5 sec
        counter++;
        if(counter == 10)
        {
            counter = 0;
            SendName();
        }
        //Draw updated screens
        if(drawChallenger)
        {
            newChallenger();
            timeout = 0;
        }
        else if(drawGame)
        {
            newGame();
            timeout = 0;
        }
        else if(drawWait1)
        {
            wait1();
            timeout = 0;
        }
        else if(drawWait2)
        {
            wait2();
            timeout = 0;
        }
        else if(drawResult)
        {
            newResult();
            timeout = 0;
        }
        else if(drawRejection)
        {
            newRejection();
            timeout = 0;
        }
        else if(drawDefault)
        {
            drawDefault = false;
            isDefault = true;
            timeout = 0;
            /* Change font color to Blue */ 
            oled.FillScreen(COLOR_BLACK);
            textProperties.fontColor   = COLOR_BLUE;
            oled.SetTextProperties(&textProperties);
            strcpy((char *) text,"JajankenGo!");
            oled.Label((uint8_t *)text,12,38);
        }
        else
            timeout++;
        //If in same state for 10s, return to default state (timeout)
        if(timeout == 400 && !isDefault)
        {
            drawChallenger = false;
            drawGame = false;
            isChallenger = false;
            isGame = false;
            isDefault = false;
            drawDefault = true;
            isWait1 = false;
            drawWait1 = false;
            isWait2 = false;
            drawWait2 = false;
            isResult = false;
            drawResult = false;
            isRejection = false;
            drawRejection = false;
            timeout = 0;
        }
        Thread::wait(50);
    }
}

/******************************End of Main*************************************/

void StartHaptic(void)  {
    hapticTimer.start(50);
    haptic = 1;
}

void StopHaptic(void const *n) {
    haptic = 0;
    hapticTimer.stop();
}

