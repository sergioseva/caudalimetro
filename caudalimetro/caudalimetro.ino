#include <MenuBackend.h>    //MenuBackend library - copyright by Alexander Brevig
#include <LiquidCrystal.h>
#include <LCDKeypad.h>
#include <Wire.h>
#include "EEPROMAnything.h"
#define LCD_Backlight 10 // 15. BL1 - Backlight + 

byte up[8] = {
    B00000,
    B00100,
    B01110,
    B11111,
    B00100,
    B00100,
    B00100,
    B00000
};
byte down[8] = {
    B00000,
    B00100,
    B00100,
    B00100,
    B11111,
    B01110,
    B00100,
    B00000
};


const int cantHistoria=25;

//parametros de configuracion
unsigned int periodo=30;
unsigned int helice=1;
float ultimaMedicion=0;
float coeficientes[] = {0.2368,0.5095,0.1167};
float suma[] = {0.0044,0.0004,0.0191};
bool midiendo;
//medicion de las rotaciones
int rot=0; 
unsigned long measureTime = 0;
unsigned long millisStart;
String  unidades= "m/s";
//brillo
byte contrast_active;
int brillo=0;
int indice=0;
LCDKeypad lcd;

//variables para el encoder
const int pinVuelta=2;                   // Used for generating interrupts using CLK signal



bool exitmenu=false;


//para almacenar mediciones
typedef struct  {
    unsigned int periodo;
    unsigned int helice;
    float medicion;
} medicionRecord;

medicionRecord mediciones[cantHistoria];


void printBrillo() {
  // Set the cursor at the begining of the second row
  lcd.setCursor(5,1);
  char cbrillo[17];
  sprintf(cbrillo, "%02u", brillo);
  lcd.print(cbrillo);
}

boolean buttonProcessBrillo(int b) {
    // Read the buttons five times in a second
    // Read the buttons value
    switch (b) {
    // Up button was pushed
    case KEYPAD_UP:
        brillo++;
        if (brillo==21) {
            brillo=1;
            contrast_active=12;
          } else {
            contrast_active+= 12;
          } 
        activarPantalla();
        break;

    // Down button was pushed
    case KEYPAD_DOWN:
      brillo--;
      if (brillo==0) {
          brillo=20;
          contrast_active=12*20;
        } else {
          contrast_active-= 12;
        }      
      activarPantalla();
      break;

      case KEYPAD_SELECT:
        //seteo la hora        
        return true;
        
    }
    activarPantalla();
    printBrillo();
    return false;
}


void mostrarDatos(int ind=999) {
    unsigned int h;
    unsigned int p;
    String m="";
    float valor;
    
    if (ind==999) { //muestro la ultima medicion
        h=helice;
        p=periodo;
        valor=ultimaMedicion;
    } else { //muestro la medicion que me pasan en el indice
        Serial.println("mostrar datos indice:"+String(ind));
        h=mediciones[ind].helice;
        Serial.println("helice"+String(h));
        p=mediciones[ind].periodo;
        valor=mediciones[ind].medicion;
        m="M"+ String(ind + 1) + " ";
    }
     lcd.setCursor(0,0); 
     String s=m +"Hel:" + String(h) + " Per:" + String(p);
     lcd.print(s);
     lcd.setCursor(0,1);
     String medicion; 
      if (midiendo) {
         //Serial.println("mostrando datos midiendo:");
        float pertemp=(millis()-millisStart)/1000;
        if (pertemp<1){
          lcd.print("Comenzando...");
          return;
          }
        //Serial.println("periodo:"+pertemp);
        float vueltasxsegundo=(float)rot/2.00/pertemp;
        valor=((vueltasxsegundo)*coeficientes[h-1])+suma[h-1];
        //Serial.println("valor:"+String(valor));
        medicion="M.. " + String(valor) + " " + unidades + "          ";
      } else {
        //poner cartel ultima medicion
        medicion=String(valor) + " " + unidades + "          ";
      }
      lcd.print(medicion);
}

boolean buttonProcessMedicion(int b) {
    // Read the buttons five times in a second
    // Read the buttons value

    //Serial.println(indice);
    switch (b) {
    // Up button was pushed
    case KEYPAD_UP:
        indice++;
        if (indice==cantHistoria) {
            indice=0;
          } 
        mostrarDatos(indice);
        break;

    // Down button was pushed
    case KEYPAD_DOWN:
      indice--;
      if (indice==-1) {
          indice=cantHistoria-1;
         
        }      
      mostrarDatos(indice);
      break;

      case KEYPAD_SELECT:
        return true;
    }
    //mostrarDatos();
    return false;
}

boolean waitReleaseButton(int lastButtonPressed,bool (*funcion_procesar_boton)(int),void (*funcion_print)())
{ int b=lastButtonPressed;
  boolean salir=false;
  do {
   // Print the time on the LCD
   funcion_print();
   salir=funcion_procesar_boton(b);
   delay(200);
   b = lcd.button();
  } while(b!=KEYPAD_NONE && !salir);
  return salir;
  
}

void configurarBrillo (){
    int buttonPressed;
      boolean salir=false;
      lcd.setCursor(0,0);
      lcd.print("Conf: Brillo   ");
       
      printBrillo();
      unsigned long starting=millis();
    
     while  (!exitmenu){
      
        do
        {
             buttonPressed=waitButton();
        } while(!(buttonPressed==KEYPAD_SELECT || buttonPressed==KEYPAD_UP || buttonPressed==KEYPAD_DOWN  ) && !exitmenu);
        
        if (!exitmenu) {
              exitmenu=waitReleaseButton(buttonPressed,buttonProcessBrillo,printBrillo);
        }
       //buttonPressed=lcd.button();  //I splitted button reading and navigation in two procedures because 
      }
 
     exitmenu=false;
  }

void resetMediciones(){
    for (int i=0;i <cantHistoria;i++) {
          mediciones[i].periodo=0;
          mediciones[i].helice=0;
          mediciones[i].medicion=0;    
    }

   helice=1;
   periodo=30;
   ultimaMedicion=0;
   EEPROM_writeAnything(0, mediciones);
}

void confirmar(String texto,void (*funcion_si)(),String conf1="",String conf2=""){
int buttonPressed;
lcd.setCursor(0,0);
lcd.print(texto);
lcd.setCursor(0,1);
lcd.write((byte)1);
lcd.setCursor(1,1);
lcd.print("si");
lcd.setCursor(4,1);
lcd.write((byte)2);
lcd.setCursor(5,1);
lcd.print("no");
    do
      {
      buttonPressed=waitButton();
      } while(!(buttonPressed==KEYPAD_UP || buttonPressed==KEYPAD_DOWN) && !exitmenu);
      
    if (!exitmenu) {
      waitReleaseButton();
      if (buttonPressed==KEYPAD_UP) {
      funcion_si();
      lcd.setCursor(0,0);
      lcd.print(conf1);
      lcd.setCursor(0,1);
      lcd.print(conf2);
      delay(3000);
      }     
    }
exitmenu=false;   
}

void menuUsed(MenuUseEvent used){
  
  String smenu=String(used.item.getName());
  lcd.setCursor(0,1); 
  lcd.print("                ");
  lcd.setCursor(0,0);  
  if (smenu.indexOf("Salir ")!=-1){
    exitmenu=true; 
  } else if  (smenu.indexOf("30")!=-1){
    Serial.println("Periodo 30");
    setearPeriodo(30);
    }  else if  (smenu.indexOf("60")!=-1){
    Serial.println("Periodo 60");
    setearPeriodo(60);
  } else if  (smenu.indexOf("90")!=-1){
    Serial.println("Periodo 90");
    setearPeriodo(90);
  } else if  (smenu.indexOf("1")!=-1){
    Serial.println("helice 1");
    setearHelice(1);
  } else if (smenu.indexOf("2")!=-1){
    Serial.println("Helice 2");
    setearHelice(2);
  } else if (smenu.indexOf("3")!=-1){
    Serial.println("Helice 3");
    setearHelice(3);
  } else if (smenu.indexOf("Ult mediciciones")!=-1){
    Serial.println("Recuperar");
    mostrarArrayMediciones();
  }  
  else if (smenu.indexOf("Comenz medicion")!=-1){
    Serial.println("Comenz medicion");
    comenzarMedicion();
  }
  else if (smenu.indexOf("Brillo")!=-1){
    Serial.println("Brillo");
    configurarBrillo();  
  }
  else if (smenu.indexOf("Reset mediciones")!=-1){
    Serial.println("Reset mediciones");
     confirmar("Confirma reset? ",resetMediciones,"Mediciones      ", "en 0           ");
  }

  exitmenu=true;
  lcd.clear();


}
 //Menu variables
    MenuBackend menu = MenuBackend(menuUsed,menuChanged);
//initialize menuitems
    MenuItem menu1Item1 = MenuItem("Comenz medicion ");
    MenuItem menu1Item2 = MenuItem("Config periodo  ");
    MenuItem menuItem2SubItem1 = MenuItem("30 segundos     ");
    MenuItem menuItem2SubItem2 = MenuItem("60 segundos     ");
    MenuItem menuItem2SubItem3 = MenuItem("90 segundos     ");
    MenuItem menuItem2SubItem4 = MenuItem("Menu anterior   ");    
    MenuItem menu1Item3 = MenuItem("Config helice   ");  
    MenuItem menuItem3SubItem1 = MenuItem("Helice 1        ");
    MenuItem menuItem3SubItem2 = MenuItem("Helice 2        ");
    MenuItem menuItem3SubItem3 = MenuItem("Helice 3        ");
    MenuItem menuItem3SubItem4 = MenuItem("Menu anterior   ");           
    MenuItem menu1Item4 = MenuItem("Ult mediciciones");
    MenuItem menu1Item5 = MenuItem("Reset mediciones");
	  MenuItem menu1Item6 = MenuItem("Brillo          ");
    MenuItem menu1Item7 = MenuItem("Salir           ");
    


void menuChanged(MenuChangeEvent changed){
  
  MenuItem newMenuItem=changed.to; //get the destination menu
  
  lcd.setCursor(0,1); //set the start position for lcd printing to the second row
  Serial.print("menuChanged");
  
  if(newMenuItem.getName()==menu.getRoot()){
      lcd.print("Main Menu       ");
  }else {
    lcd.print(newMenuItem.getName());
    }
  
  
}

 void addRotation() {
     if (midiendo) //&& (millis()-millisStart)/1000<=periodo)
        {
        rot++;}
      Serial.println("rot:"+ String(rot));  

  }

void setup () {
   Serial.begin(9600);
   Wire.begin();

   //pantalla
   contrast_active=128;
   brillo=contrast_active/12;
   //medicion 
   pinMode(pinVuelta, INPUT); 
   attachInterrupt(digitalPinToInterrupt(pinVuelta), addRotation, RISING);

     //configure menu
    menu.getRoot().add(menu1Item1);
    menu1Item1.addRight(menu1Item2).addRight(menu1Item3).addRight(menu1Item4).addRight(menu1Item5).addRight(menu1Item6).addRight(menu1Item7);
    menu1Item2.addAfter(menuItem2SubItem4);//para que al elegir subir vaya al submenu
    menu1Item2.add(menuItem2SubItem1).addRight(menuItem2SubItem2).addRight(menuItem2SubItem3).addRight(menuItem2SubItem4);
    menu1Item3.addAfter(menuItem3SubItem4);//para que al elegir subir vaya al submenu
    menu1Item3.add(menuItem3SubItem1).addRight(menuItem3SubItem2).addRight(menuItem3SubItem3).addRight(menuItem3SubItem4);
    menu.toRoot();

    //pantalla
    lcd.begin(16, 2);
    lcd.clear();
    //ver si es necesario la pantalla en stand by

    midiendo=false;


   int count=EEPROM_readAnything(0, mediciones);
   Serial.println(count);
   Serial.println(mediciones[0].periodo);
   ultimaMedicion=mediciones[0].medicion;
   helice=mediciones[0].helice;
   periodo=mediciones[0].periodo;

   lcd.createChar(1, up); 
   lcd.createChar(2, down); 
}



int waitButton()
{
  int buttonPressed; 
  unsigned long starting=millis();
  unsigned long current;
  while((buttonPressed=lcd.button())==KEYPAD_NONE)
  {
    current=millis();
    //controlo que no pase mas de un minuto en espera, sino salgo
    if ((current-starting)>60000) {
          exitmenu=true;
          break;
          }
  }
  delay(50);  
  lcd.noBlink();
  return buttonPressed;
}

void waitReleaseButton()
{
  delay(50);
  while(lcd.button()!=KEYPAD_NONE)
  {
  }
  delay(50);
}


void navigateMenus(int b) {
  MenuItem currentMenu=menu.getCurrent();
  Serial.print("button:");
  Serial.println(b);
  switch (b){
    case KEYPAD_SELECT:
      if(!(currentMenu.moveDown() || currentMenu.getName()=="Menu anterior   ")){  //if the current menu has a child and has been pressed enter then menu navigate to item below
        menu.use();
      }else if (currentMenu.getName()=="Menu anterior   ") {
        menu.moveUp();
         Serial.print("moveUp");
        }      
      else {  //otherwise, if menu has no child and has been pressed enter the current menu is used
        menu.moveDown();
       } 
      break;
    case KEYPAD_RIGHT:
    Serial.println("moveRight");
      menu.moveRight();
      break;      
    case KEYPAD_LEFT:
    Serial.println("moveLeft");
      menu.moveLeft();
      break;      
  }
}

void setearPeriodo(int p) {
    Serial.println("periodo");
    Serial.println(p);
    periodo=p;
}

void setearHelice(int h) {
    helice=h;
}


void activarPantalla(){
    analogWrite(LCD_Backlight, contrast_active);  
}

void procesarMenu(){
  int buttonPressed=lcd.button();

  if (buttonPressed==KEYPAD_SELECT && !midiendo){
    waitReleaseButton();
    lcd.setCursor(0,0);  
    lcd.print("Menu            ");
    exitmenu=false;
    menu.toRoot();
    menu.moveDown();
    buttonPressed=KEYPAD_NONE;
  
    while  (!exitmenu){
      while(!(buttonPressed==KEYPAD_SELECT || buttonPressed==KEYPAD_LEFT || buttonPressed==KEYPAD_RIGHT) && !exitmenu)
      {     
        buttonPressed=waitButton();     
      } 
      if (!exitmenu) {
      waitReleaseButton();
      navigateMenus(buttonPressed);  //in some situations I want to use the button for other purpose (eg. to change some settings)
      }
    buttonPressed=KEYPAD_NONE;  
    }
  buttonPressed=KEYPAD_NONE;  
  //activarPantalla();	
}
  
}

void comenzarMedicion() {

    
    //comienzo a medir
    rot=0;
    millisStart=millis();
    midiendo=true;

}  

void moverUltimaMedicion() {
  for (int i=cantHistoria-1;i >0;i--) {
    mediciones[i].periodo=mediciones[i-1].periodo;
    mediciones[i].helice=mediciones[i-1].helice;
    mediciones[i].medicion=mediciones[i-1].medicion;    
  }
}


void finalizarMedicion() {
    midiendo=false;
    float vueltasxsegundo=(float)rot/2.00/(float)periodo;
    Serial.println("Vueltas x segundo:"+String(vueltasxsegundo));
    ultimaMedicion=((vueltasxsegundo)*coeficientes[helice-1])+suma[helice-1];
     //Almaceno la medicion 
    moverUltimaMedicion();
    mediciones[0].periodo=periodo;
    mediciones[0].helice=helice;
    mediciones[0].medicion=ultimaMedicion;
    EEPROM_writeAnything(0, mediciones);
}  

void mostrarArrayMediciones() {

      int buttonPressed;
      boolean salir=false;
      lcd.setCursor(0,0);
      mostrarDatos(0);
      unsigned long starting=millis();
    
     while  (!exitmenu){
        do
        {
             buttonPressed=waitButton();
        } while(!(buttonPressed==KEYPAD_SELECT || buttonPressed==KEYPAD_UP || buttonPressed==KEYPAD_DOWN  ) && !exitmenu);
        
        if (!exitmenu) {
              exitmenu=waitReleaseButton(buttonPressed,buttonProcessMedicion,mostrarDatos);
        }
       //buttonPressed=lcd.button();  //I splitted button reading and navigation in two procedures because 
      }
 
     exitmenu=false;
}
void loop() {
	   
     if (midiendo && (millis()-millisStart)/1000>(periodo)){
         finalizarMedicion();
         Serial.println("finalizo medicion: " + String(periodo));
         //Serial.println("tiempo transcurrido en millisegundos:" + String((millis()-millisStart)));
     }
     mostrarDatos();  // Escribimos el valor en pantalla
     procesarMenu();

}
