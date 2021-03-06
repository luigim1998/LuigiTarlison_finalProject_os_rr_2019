#include "cMIPS.h"

typedef struct control {   // control register fields (uses only ls byte)
    int ign   : 24+3,      // ignore uppermost bits
        intTX : 1,         // interrupt on TX buffer empty (bit 4)
        intRX : 1,         // interrupt on RX buffer full (bit 3)
        speed : 3;         // 4,8,16..256 tx-rx clock data rates  (bits 0..2)
} Tcontrol;

typedef struct status {    // status register fields (uses only ls byte)
    int s;
    // int ign   : 24,     // ignore uppermost bits
    //  ign7    : 1,       // ignored (bit 7)
    //  txEmpty : 1,       // TX register is empty (bit 6)
    //  rxFull  : 1,       // octet available from RX register (bit 5)
    //  int_TX_empt: 1,    // interrupt pending on TX empty (bit 4)
    //  int_RX_full: 1,    // interrupt pending on RX full (bit 3)
    //  ign2    : 1,       // ignored (bit 2)
    //  framing : 1,       // framing error (bit 1)
    //  overrun  : 1;      // overrun error (bit 0)
} Tstatus;

#define RXfull  0x00000020
#define TXempty 0x00000040

typedef union ctlStat {    // control + status on same address
    Tcontrol  ctl;         // write-only
    Tstatus   stat;        // read-only
} TctlStat;

typedef union data {       // data registers on same address
    int tx;                // write-only
    int rx;                // read-only
} Tdata;

typedef struct serial {
    TctlStat cs;           // control & status at address UART + 0
    Tdata    d;            // TX & RX registers at address UART + 4
} Tserial;

typedef struct UARTDriver{
    char rx_q[16];         // reception queue
    int rx_hd;             // reception queue head index
    int rx_tl;             // reception queue tail index
    char tx_q[16];         // transmission queue
    int tx_hd;             // transmission queue head index
    int tx_tl;             // transmission queue tail index
    int nrx;               // characters in RX_queue
    int ntx;               // spaces left in TX_queue
} UARTDriver;

#define EOF -1
#define MAX_SIZE 16

int proberx(void);         // returns nrx
int probetx(void);         // returns ntx
int iostat(void);          // returns integer with status at lsb
void ioctl(int);           // write lsb in control register
char getc(void);           // returns char in queue, decrements nrx
void Putc(char);           // inserts char in queue, decrements ntx

void initUd();
void insertionSort(int v[], int size);
void writeFirstChar();     // get the first character on TX queue and write it in the UART
int fibonacci(int entrada);// fibonacci function
int ctoi(char);            // converts a character to an integer
void itoc(int, char*);     // converts integer to char 

extern UARTDriver Ud;
volatile Tserial *uart;

int main(){
    uart = (void *)IO_UART_ADDR; // bottom of UART address range
    int ictl = 0x1a; // 00011010: ign=0; intTX=1; intRX=1; speed=2;
    ioctl(ictl);

    initUd();
    volatile char c;
    int h, i=0, n=0;
    int fib[MAX_SIZE];
    char last = EOF;
    while((c = getc()) != '\n' || last != '\n') {
        if(c != EOF) {
            if(c == '\n' && last != '\n') {
            	fib[i] = fibonacci(n);
                i++;
                n = 0;
            }else{
                h = ctoi(c);
                if(h != EOF) {
                    n = n*16 + h;         
                }
            }
            last = c;
        }
    }

    char hex[9];
    int count;
    for(count = 0; count < i; count++){
        itoc(fib[count], hex);
        n = 0;
        while(hex[n] != '\0') {
            Putc(hex[n]);
            n++;
        }
        Putc('\n');
    }
    
    if(probetx() < 16){
        writeFirstChar();
    }

    for(count=0;count<1000;count++);  //Wait for the remote uart
    
    return 0;
}

void initUd(){
    Ud.rx_hd = 0;
    Ud.rx_tl = 0;
    Ud.tx_hd = 0;
    Ud.tx_tl = 0;
    Ud.nrx = 0;
    Ud.ntx = 16;
}

int proberx(){
    return Ud.nrx;
}

int probetx(){
    return Ud.ntx;
}

int iostat(){
    return uart->cs.stat.s;
}

void ioctl(int ictl){
    Tcontrol ctrl;
    ctrl.speed = ictl & 0x7; 
    ctrl.intRX = (ictl>>3) & 0x1;
    ctrl.intTX = (ictl>>4) & 0x1;
    ctrl.ign   = (ictl>>5) & 0x7;
    uart->cs.ctl = ctrl;
}

char getc(){
    char c = EOF;
    if(proberx() > 0){
        disableInterr();
        c = Ud.rx_q[Ud.rx_hd];
        Ud.rx_hd = (Ud.rx_hd+1)%16;
        Ud.nrx--;
        enableInterr();
    }
    return c;
}

void Putc(char c){
    if(probetx() > 0){
        disableInterr();
        Ud.tx_q[Ud.tx_tl] = c;
        Ud.tx_tl = (Ud.tx_tl+1)%16;
        Ud.ntx--;
        enableInterr();
    }else{
        writeFirstChar();
        Putc(c);
    }
}

void writeFirstChar(){
    while(!(iostat()&TXempty));
    disableInterr();
    uart->d.tx = Ud.tx_q[Ud.tx_hd];
    Ud.tx_hd = (Ud.tx_hd+1)%16;
    Ud.ntx++;
    enableInterr();
}

int fibonacci(int entrada){
    if(entrada == 0 || entrada == 1){
        return 1;
    } else {
        int a = 1, b = 1;
        int c;
        for (int cont = 2; cont <= entrada; cont++){
            c = a + b;
            a = b;
            b = c;
        }
        return c;
    }
}

int ctoi(char c) {
    // If it's a number
    if(c >=0x30 && c < 0x3a) {
        return ((int) c) - 0x30;
    }

    // If it's an uppercase letter
    if(c >= 0x41 && c < 0x47) {
        return ((int) c) - 0x37; // 0x41 - 0xa
    }

    // If it's a lowercase letter
    if(c >= 0x61 && c < 0x67) {
        return ((int) c) - 0x57; //0x61 - 0xa
    }

    return EOF;
}

void itoc(int n, char* result) {
    int i, hex, j;
    char c;
    j = 0;
    result[0] = '\0';
    for(i = 28; i >= 0; i-=4) {
        hex = (n>>i)&0xf;

        if(hex >= 0 && hex < 10) {
            c = (char) hex + 0x30;
        }else if(hex >= 10 && hex < 16) {
            c = (char) hex + 0x37;
        }

        if(hex == 0 && result[0] != '\0') {
            result[j] = c;
            j++;
        } else if(hex != 0) {
            result[j] = c;
            j++;
        }
    }
    if(result[0] == '\0') {
        result[0] = '0';
        j = 1;
    }
    result[j] = '\0';
}
