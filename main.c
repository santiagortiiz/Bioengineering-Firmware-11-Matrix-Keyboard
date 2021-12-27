/* ========================================
 *
 * Copyright YOUR COMPANY, THE YEAR
 * All Rights Reserved
 * UNPUBLISHED, LICENSED SOFTWARE.
 *
 * CONFIDENTIAL AND PROPRIETARY INFORMATION
 * WHICH IS THE PROPERTY OF your company.
 *
 * ========================================
*/
#include "project.h"

#define and &&
#define or ||

#define variable Variables_1.Variable_1

unsigned char tecla;
unsigned char claveOriginal[4] = {'3','8','4','0'};                             // Vectores de acceso al sistema
unsigned char claveIngresada[4] = {'*','*','*','*'};

uint8 Led[9] = {0xFF, 0x7F, 0x3F, 0x1F, 0xF, 0x7, 0x3, 0x1, 0};                 // Se crea un vector con los valores que 
                                                                                // apagan los leds uno por uno cada 500ms

// ------------------------------------------

typedef struct Tiempo{                                                          // Estructura con las variables de tiempo
    uint64 ms:10;                                                               // que permiten controlar freuencia
    uint64 seg:6;
    uint64 periodo:10;
}Tiempo;
Tiempo tiempo;

typedef union Banderas_1{                                                       // Unión entre una variable de reseteo
    struct Variables1{                                                          // y variables generales para el control
        uint16 teclaPresionada:1;                                               // del sistema
        uint16 posicion:3;
        uint16 estado:3;
        uint16 intentos:2;
        uint16 contador:1;
        uint16 unidadesPeso:1;
        uint16 unidadesTemp:1;
        uint16 farenheit0:1;
        uint16 signo:1;
        uint16 cambiarClave:1;
        uint16 salirPresionado:1;
    }Variable_1;
    uint16 resetear;
}banderas_1;
banderas_1 Variables_1;

typedef struct Medidas{                                                         // Estructura con las variables de sensado y su rango
    uint32 peso:14;                                                             // [0 : 5000]
    uint32 temperatura:14;                                                      // [0 : 1100]
}medidas;
medidas medida;


void menu(void);                                                                // Rutinas del sistema
void ingresarClave(void);
void compararClave(const uint8 *ingresada, const uint8 *original);
void claveCambiada(void);
void sensar(void);
void parpadear(float frecuencia);
void reset(void);
void salirSistema(void);

CY_ISR_PROTO(botonSalir); 
CY_ISR_PROTO(leerTecla);
CY_ISR_PROTO(contador);  

int main(void)
{
    CyGlobalIntEnable; /* Enable global interrupts. */
    isr_botonSalir_StartEx(botonSalir);                                         // Interrupciones
    isr_KBI_StartEx(leerTecla);
    isr_contador_StartEx(contador);
                                                                      
    Contador_Start();                                                           // Componentes
    LCD_Start();    
    ADC_Start();
    AMux_Start();
    
    LCD_LoadCustomFonts(LCD_customFonts);                             
    LCD_ClearDisplay();   
    
    variable.estado = 0;                                                        // Inicio
    menu();
    for(;;)
    {
        if (variable.teclaPresionada == 1){                                     // Ingresa a este condicional siempre
            variable.teclaPresionada = 0;                                       // que se presione una tecla
            tecla = Teclado_teclaPresionada();
            
            switch (variable.estado){
                case 0:                                                         // ESTADO = 0
                    reset();                                                    // Menu de bienvenida
                    menu();
                    break;
                case 1:                                                         // ESTADO = 1
                    ingresarClave();                                            // Ingreso al sistema
                    break;
                case 2:                                                         // ESTADO = 2
                    if(tecla == '1') variable.estado = 3;                       // Menu Principal
                    else if(tecla == '2') {                                     // 1 - Visualizacion   2 - Cambiar clave
                        variable.estado = 4;                                    // '#' Regresa al menu de ingreso al sistema
                        menu();                                                 // NOTA: Si se presiona '2' el estado del sistema pasa
                    }                                                           // a 4, y se llama al menu, el cual pide al usuario 
                    else if(tecla == '#') {                                     // la clave original y redirecciona el sistema al estado 5
                        variable.estado = 0;
                        variable.teclaPresionada = 1;
                    }
                    break;
                case 3:                                                         // ESTADO = 3
                    if(tecla == 'A') variable.unidadesPeso = ~variable.unidadesPeso;
                    else if(tecla == 'B') variable.unidadesTemp = ~variable.unidadesTemp;
                    else if(tecla == '#') {                                     // 'A' y 'B' cambian las unidades de medida
                        variable.estado = 2;                                    // '#' Regresa al menu principal
                        menu();
                    }
                    break;
                case 5:                                                         // ESTADO = 5 y 6: CAMBIAR CLAVE
                    ingresarClave();                                            // 5 -> Ingresa la clave original
                    break;
                case 6:                                                         // 6 -> Ingresa la clave nueva
                    ingresarClave();
                    break;
                    
            }
        }
        
        if(variable.estado == 3 and tiempo.ms%500 == 0){                        // Rutina de sensado para el ESTADO = 3
            sensar();
        }
        
        while(variable.cambiarClave == 1){                                      // Rutina de CLAVE CAMBIADA CON EXITO
            
            if(tiempo.ms%82 == 0){                                              // Sí la clave se cambio correctamente
                buzzer_Write(1);                                  // se activan 3 PITIDOS en medio segundo
            }                                                                   // y luego se muestra el menu principal
            if(tiempo.ms%166 == 0) buzzer_Write(0);
            if(tiempo.ms > 500) {
                buzzer_Write(0);
                variable.cambiarClave = 0;
                variable.estado = 2;
                for (uint8 posicion = 0; posicion < 4; posicion++) claveIngresada[posicion] = '*'; 
                variable.posicion = 0;
                menu();
            }
        }
                                                                        
        if(variable.salirPresionado == 1 and BotonSalir_Read() == 0)variable.salirPresionado = 0;
        if(variable.salirPresionado == 1 and tiempo.seg > 3)salirSistema();     // RUTINA DE SALIDA DEL SISTEMA
        
    }
}

void menu(void){
    switch (variable.estado){                                                   // INICIO DEL SISTEMA 
            case 0:                                                             // mensaje de bienvenida
                LCD_ClearDisplay();
                LCD_Position(0,5);
                LCD_PrintString("Bienvenido");
                LCD_Position(1,8);
                LCD_PrintString("****");
                variable.estado = 1;
                break;
                
            case 1:
                LCD_Position(1,variable.posicion+8);                            // Se escriben los caracteres ingresados
                LCD_PutChar(tecla);                                             // por el usuario en el ingreso de la clave
                variable.posicion++;
                break;
                
            case 2:                                                             // MENU PRINCIPAL   
                LCD_ClearDisplay();
                LCD_Position(1,0);
                LCD_PrintString("1-Visualizacion");
                LCD_Position(2,0);
                LCD_PrintString("2-Cambiar clave");
                break;
                
            case 3:                                                             // VISUALIZACION
                LCD_ClearDisplay();
                LCD_Position(0,0); 
                LCD_PrintString("Sistema de medicion");
                
                                                                                // Rutina de impresion para el PESO:
                LCD_Position(1,0); LCD_PrintString("Peso =");                   // Si unidadesPeso = 0 se imprimen tal 
                LCD_Position(1,14);LCD_PrintString("Kg");                       // y como el algoritmo lee la variable, 
                                                                                // pero de ser 1, se realiza la conversion a libras
                if(variable.unidadesPeso == 1){
                    medida.peso = 2.2*medida.peso;
                    LCD_Position(1,14);LCD_PrintString("lbs");
                }
                
                LCD_Position(1,7);
                LCD_PrintNumber(medida.peso/100);
                LCD_PutChar('.');
                if (medida.peso%100 < 10) LCD_PrintNumber(0);
                LCD_PrintNumber(medida.peso%100);
                
                                                                                // Rutina de impresion para la TEMPERATURA:
                LCD_Position(2,7);                                              // Si unidadesTemp = 0, se imprime tal y como se lee
                if(variable.unidadesTemp == 0){                                 // la variable, de lo contrario se hace la conversion 
                    LCD_PrintNumber(medida.temperatura/100);                    // a °C con ayuda de farenheit0
                    LCD_PutChar('.');
                    LCD_PrintNumber((medida.temperatura/10)%10);
                    LCD_Position(2,14);LCD_PutChar(LCD_CUSTOM_0);LCD_PrintString("F");
                    
                }
                
                //LCD_Position(3,0);LCD_PrintNumber(medida.temperatura);          
                if(variable.unidadesTemp == 1){                                 // NOTA: Sí el usuario escoge visualizar T -> °C 
                                                                                // se realizan 3 divisiones para imprimir la conversion:
                    if (medida.temperatura < 3200){
                        if(variable.farenheit0 == 0){                           // 1. 0 < °F < 32
                            medida.temperatura = 0.55555555*(3200 - medida.temperatura);
                            variable.signo = 1;
                        }
                        else{                                                   // 2. °F < 0
                            medida.temperatura = 0.55555555*(3200 + medida.temperatura);
                            variable.signo = 1;                                 // Detalle 1: en ambos rangos la temperatura en °F leida esta
                        }                                                       // entre 0 y 32 (positivo), por lo que debía determinarse un
                    }                                                           // evento que diferenciará cuando se media °F negativo de positivo
                                                                                // por ejemplo: -10°F de 10°F, ya que en ambos el valor resultante
                                                                                // de la funcion sensar, es positivo, y solo se añade un signo - 
                                                                                // al imprimir, pero su valor es el mismo, para esto se uso "farenheit0"
                    
                    else{                                                       // 3. °F > 32     
                        medida.temperatura = 0.555555555*(medida.temperatura - 3200);
                        variable.signo = 0;
                    }
                                                                                // Detalle 2: Para tomar 2 decimales de forma precisa, el calculo
                    LCD_Position(2,7);                                          // de la temperatura para se FARENHEIT, se realizo con 2 decimales
                    LCD_PrintNumber(medida.temperatura/100);                    // pero solo se imprime 1, de esta forma, al hacer la conversión a
                    LCD_PutChar('.');                                           // °C, ésta quedará con 2 decimales, y se podran imprimir directamente
                    if(medida.temperatura%100 < 10) LCD_PrintNumber(0);         // lo cual facilita la rutina de impresion
                    LCD_PrintNumber(medida.temperatura%100);
                    LCD_Position(2,14);LCD_PutChar(LCD_CUSTOM_0);LCD_PrintString("C");
                }                                                               // Detalle 3: Para mostrar los decimales, se usa el módulo de
                                                                                // la variable, sin embargo cuando ésta es < 10, se imprime un "0"
                LCD_Position(2,0);                                              // previamente, para continuar mostrando 2 cifras decimales
                if(variable.signo == 0) LCD_PrintString("Temp = ");
                else if(variable.signo == 1) LCD_PrintString("Temp =-");
                
                break;
            case 4:
                LCD_ClearDisplay();                                             // CAMBIAR DE CLAVE
                LCD_Position(1,4);LCD_PrintString("Clave Actual");              // 1. Se pide al usuario ingresar la clave actual
                LCD_Position(2,8);LCD_PrintString("____");
                variable.estado = 5;
                break;
            case 5:                                                             // 2. Se escriben los caracteres ingresados
                LCD_Position(2,variable.posicion+8);
                LCD_PutChar(tecla);
                variable.posicion++;
                break;
            case 6:
                if(tecla != '#' and tecla != '*'){                              // 3. Clave Nueva
                    LCD_Position(2,variable.posicion+8);                        // Sólo se escribe un caracter si es diferente
                    LCD_PutChar(tecla);                                         // de las teclas ya usadas para confirmar o retroceder
                    variable.posicion++;                                        // en el menu
                }
                break;
    }
}

void sensar(void){
    AMux_Select(0);                                                             // Rutina de sensado para el PESO
    ADC_StartConvert();                                                         
    ADC_IsEndConversion(ADC_WAIT_FOR_RESULT);
    
    if (ADC_GetResult16() <= 3890.25){
        medida.peso = 100*ADC_GetResult16()*50/3890.25;                         // peso[kg] = 2_decimales * bitsADC * ecuacionPeso
    }                                                                          
    else medida.peso = 100*50;
    
    AMux_Select(1);                                                             // Rutina de sensado para la TEMPERATURA
    ADC_StartConvert();
    ADC_IsEndConversion(ADC_WAIT_FOR_RESULT);
                                                                                // farenheit0 determina un cambio en la ecuacion de temperatura [°F]
    variable.farenheit0 = 0;                                                    // variable positiva (farenheit0 = 0) o negativa (farenheit0 = 1)
    
    if (ADC_GetResult16() >= 1228.5 and ADC_GetResult16() <= 3931.2) {
        medida.temperatura = (uint32)100*(ADC_GetResult16() - 958.23)*100/2702.7;
        variable.signo = 0;                                                     // Imprime valor positivo
    }                                                                           // temp[°F] = 2_decimales * bitsADC * ecuacionTemp
    else if (ADC_GetResult16() > 3931.2) {
        medida.temperatura = 110*100; 
        variable.signo = 0;
    }
    else{
        if (ADC_GetResult16() >= 819){
            medida.temperatura = (uint32)100*(ADC_GetResult16() - 819)*30/1228.5;
            variable.signo = 0;                                                 // Imprime valor positivo
            
        }
        else{
            medida.temperatura = (uint32)100*(819 - ADC_GetResult16())*30/1228.5;
            variable.signo = 1;                                                 // Imprime valor negativo
            variable.farenheit0 = 1;                                            // Se activa farenheit0 = 1 para permitir calcular
        }                                                                       // la conversion a °C.
    }
    
    menu();
}

void compararClave(const uint8 *ingresada, const uint8 *original){
    uint8 aciertos = 0;                                                         // Esta funcion recibe la dirección en memoria
                                                                                // de los vectores clave original e ingresada,
    for (uint8 i = 0; i < 4; i++){                                              // los compara posicion a posicion, e incrementa 
        if (*ingresada == *original) aciertos++;                                // la variable "aciertos" cuando ambos coinciden
        ingresada++;                                                            // Y EJECUTA UNA ACCION EN FUNCION DEL ESTADO DEL
        original++;                                                             // SISTEMA: (estado = 1 - ingreso al sistema),
    }                                                                           // (estado = 5 - cambiar clave)
    
    switch(variable.estado){
        case 1:                                                                 // ESTADO = 1:
            if (aciertos == 4){                                                 // Sí los aciertos son 4, el sistema avanza
                variable.estado = 2;                                            // al estado siguiente
                for (uint8 posicion = 0; posicion < 4; posicion++) claveIngresada[posicion] = '*';
                variable.posicion = 0;
                menu();
            }
            else{                                                               // De lo contrario, cuenta el numero de intentos
                variable.intentos++;                                            // realizados. E imprime siempre "clave incorrecta",
                LCD_Position(2,2);                                              // reinicia el vector "claveIngresada" y la 
                LCD_PrintString("Clave Incorrecta");                            // variable posicion
                variable.posicion = 0;
                variable.estado = 0;
                for (uint8 posicion = 0; posicion < 4; posicion++) claveIngresada[posicion] = '*';
                CyDelay(1000);
                menu();
                if (variable.intentos == 3){                                    // Sí los intentos fueron 3, se presenta el mensaje
                    LCD_ClearDisplay();                                         // sistema bloqueado. 
                    LCD_Position(0,7); LCD_PrintString("Sistema");
                    LCD_Position(1,6); LCD_PrintString("bloqueado");
                    tiempo.seg = 0;
                    tiempo.periodo = 0;
                    tiempo.ms = 0;
                    while(tiempo.seg < 4){                                      // El mensaje parpadea durante 3 segundos a 2 Hz
                        parpadear(2);
                    }
                    LCD_Wakeup();                                               // Luego se enciende el buzzer 
                    buzzer_Write(1);
                    while(tiempo.seg < 6);                                      // El cual suena hasta que transcurra 1 segundo
                    buzzer_Write(0);
                    variable.estado = 0;                                        // y luego el sistema se reinicia
                    variable.teclaPresionada = 1;
                }
            }
            break;
            
        case 5:                                                                 // ESTADO = 5:
            if (aciertos == 4){                                                 // Sí los aciertos son 4, el sistema avanza
                LCD_ClearDisplay();                                             // al estado siguiente pidiendo al usuario
                LCD_Position(1,4);LCD_PrintString("Nueva Clave");               // que ingrese la NUEVA CLAVE
                LCD_Position(2,8);LCD_PrintString("____");
                variable.posicion = 0;
                for (uint8 posicion = 0; posicion < 4; posicion++) claveIngresada[posicion] = '*';
                variable.estado = 6;
            }
            else{
                LCD_Position(1,2); LCD_PrintString("Clave Incorrecta");         // De lo contrario, se muestra el mensjae intente
                LCD_Position(2,2); LCD_PrintString("Intente de nuevo");         // de nuevo por 2 segundos
                variable.estado = 4;
                for (uint8 posicion = 0; posicion < 4; posicion++) claveIngresada[posicion] = '*';
                variable.posicion = 0;
                tiempo.seg = 0;
                tiempo.ms = 0;
                CyDelay(2000);
                menu();
            }
            break;
    }
}

void ingresarClave(void){
    if (tecla == '*' and claveIngresada[3] != '*'){                             // Si se presiona el '*', se verifica que la 
        if(variable.estado != 6) {                                              // ultima posicion del vector claveIngresada
            compararClave(claveIngresada, claveOriginal);                       // sea diferente de '*' para confirmar la clave.
        }                                                                       // Luego se verifica el estado del sistema para
        else if(variable.estado == 6){                                          // determinar si el usuario esta ingresando al sistema
            for (uint8 posicion = 0; posicion < 4; posicion++) claveOriginal[posicion] = claveIngresada[posicion]; // o cambiando la clave
            variable.cambiarClave = 1;
            tiempo.ms = 0;                                                      
        }
    }  
    
    else if (tecla == '#'){                                                     // Si se presiona '#', se lee el estado del sistema
        if(variable.estado == 1) variable.estado = 0;                           // para determinar si el usuario desea borrar lo escrito
        else if(variable.estado == 5) {                                         // o regresar en el menu. Sin embargo a este condicional
            reset();                                                            // ingresa en cada estado en el que se el usuario escribe
            variable.estado = 2;                                                // una contraseña, por lo que siempre se resetea el vector
        }                                                                       // claveIngresada
        if(variable.estado == 6){
            LCD_ClearDisplay();                                                 
            LCD_Position(1,4);LCD_PrintString("Nueva Clave");
            LCD_Position(2,8);LCD_PrintString("____");
        }
        for (uint8 posicion = 0; posicion < 4; posicion++) claveIngresada[posicion] = '*'; 
        variable.posicion = 0;
        menu();
    }
    
    else if (tecla != '*' and tecla != '#'){                                    // Si se presiona cualquier otra tecla alfanumerica,
        if (variable.posicion == 4) variable.posicion = 0;                      // se asigna al vector claveIngresada en la posicion 
        claveIngresada[variable.posicion] = tecla;                              // actual y se sobre-escribe en el LCD al llamar menu
        menu();
    }
    
}

void parpadear (float frecuencia){      
    uint16 periodo_ms;
    periodo_ms = (1/frecuencia)*1000;                                           // Se calcula el período de la señal
                                                                        
    if (tiempo.periodo == periodo_ms/2){                                        // Cada que los milisegundos sean iguales
        tiempo.periodo = 0;                                                     // a la mitad del periodo de la señal, se 
        variable.contador = ~variable.contador;                                 // cambia el estado del LCD
        
        if (variable.contador == 0) LCD_Sleep();
        if (variable.contador == 1) LCD_Wakeup();
    }
}

void reset(void){                                                               // Permite resetear las variables de control del sistema
    for (uint8 posicion = 0; posicion < 4; posicion++) claveIngresada[posicion] = '*';
    Variables_1.resetear = 0;
}

void salirSistema(void){
    LCD_ClearDisplay();                                                         // Cuando el boton salir se presiona por 4 segundos
    LCD_Position(0,4);LCD_PrintString("Saliendo del");                          // se presenta un mensaje tambien 4 segundos, al 
    LCD_Position(1,6);LCD_PrintString("sistema");                               // mismo tiempo que se encienden 8 leds y se apagan
    uint8 posicion = 0;                                                         // uno a uno en este lapso de tiempo
    tiempo.seg = 0;
    tiempo.periodo = 0;
    
    while(tiempo.seg < 5){
        if(tiempo.periodo == 500){
            tiempo.periodo = 0;
            leds_Write(Led[posicion]);
            posicion++;
        }
    }
    LCD_ClearDisplay();
    reset();
    menu();
    
}

CY_ISR(leerTecla){                                                              // Interrupcion del teclado matricial
    variable.teclaPresionada = 1;
}

CY_ISR(contador){                                                               // Permite controlar frecuencias     
    tiempo.ms++;                                                                         
    tiempo.periodo++;
    if (tiempo.ms == 1000) {
        tiempo.ms = 0;
        tiempo.seg++;
        if (tiempo.seg == 60) tiempo.seg = 0;
    }
}

CY_ISR(botonSalir){                                                             // Interrupcion que reinicia los segundos del tiempo 
    if(variable.estado > 1){                                                    // y activa la variable salirPresionado
        tiempo.seg = 0;
        variable.salirPresionado = 1;
    }
}

/* [] END OF FILE */
