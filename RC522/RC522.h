#ifndef rfid_h
#define rfid_h
#include <stdint.h>
#include "spi.h"


typedef unsigned char byte;
#define _resetPin 8
#define _resetPowerDownPin ( 1 << _resetPin )


#define _true  1
#define _false 0


typedef struct {
    byte            size;                           // Number of bytes in the UID. 4, 7 or 10.
    byte            uidByte[10];
    byte            sak;                            // The SAK (Select acknowledge) byte returned from the PICC after successful selection.
} Uid;


// Member variables
//Uid uid;                                          // Used by PICC_ReadCardSerial().


typedef enum {
    PCD_Idle                        = 0x00,         // no action, cancels current command execution
    PCD_Mem                         = 0x01,         // stores 25 bytes into the internal buffer
    PCD_GenerateRandomID            = 0x02,         // generates a 10-byte random ID number
    PCD_CalcCRC                     = 0x03,         // activates the CRC coprocessor or performs a self-test
    PCD_Transmit                    = 0x04,         // transmits data from the FIFO buffer
    PCD_NoCmdChange                 = 0x07,         // no command change, can be used to modify the CommandReg register bits without affecting the command, for example, the PowerDown bit
    PCD_Receive                     = 0x08,         // activates the receiver circuits
    PCD_Transceive                  = 0x0C,         // transmits data from FIFO buffer to antenna and automatically activates the receiver after transmission
    PCD_MFAuthent                   = 0x0E,         // performs the MIFARE standard authentication as a reader
    PCD_SoftReset                   = 0x0F          // resets the MFRC522
} PCD_Command;


typedef enum {
// The commands used by the PCD to manage communication with several PICCs (ISO 14443-3, Type A, section 6.4)
    PICC_CMD_REQA                   = 0x26,         // REQuest command, Type A. Invites PICCs in state IDLE to go to READY and prepare for anticollision or selection. 7 bit frame.
    PICC_CMD_WUPA                   = 0x52,         // Wake-UP command, Type A. Invites PICCs in state IDLE and HALT to go to READY(*) and prepare for anticollision or selection. 7 bit frame.
    PICC_CMD_CT                     = 0x88,         // Cascade Tag. Not really a command, but used during anti collision.
    PICC_CMD_SEL_CL1                = 0x93,         // Anti collision/Select, Cascade Level 1
    PICC_CMD_SEL_CL2                = 0x95,         // Anti collision/Select, Cascade Level 2
    PICC_CMD_SEL_CL3                = 0x97,         // Anti collision/Select, Cascade Level 3
    PICC_CMD_HLTA                   = 0x50,         // HaLT command, Type A. Instructs an ACTIVE PICC to go to state HALT.
    PICC_CMD_RATS                   = 0xE0,         // Request command for Answer To Reset.
    PICC_CMD_MF_AUTH_KEY_A          = 0x60,         // Perform authentication with Key A
    PICC_CMD_MF_AUTH_KEY_B          = 0x61,         // Perform authentication with Key B
    PICC_CMD_MF_READ                = 0x30,         // Reads one 16 byte block from the authenticated sector of the PICC. Also used for MIFARE Ultralight.
    PICC_CMD_MF_WRITE               = 0xA0,         // Writes one 16 byte block to the authenticated sector of the PICC. Called "COMPATIBILITY WRITE" for MIFARE Ultralight.
    PICC_CMD_MF_DECREMENT           = 0xC0,         // Decrements the contents of a block and stores the result in the internal data register.
    PICC_CMD_MF_INCREMENT           = 0xC1,         // Increments the contents of a block and stores the result in the internal data register.
    PICC_CMD_MF_RESTORE             = 0xC2,         // Reads the contents of a block into the internal data register.
    PICC_CMD_MF_TRANSFER            = 0xB0,         // Writes the contents of the internal data register to a block.
    PICC_CMD_UL_WRITE               = 0xA2          // Writes one 4 byte page to the PICC.
} PICC_Command;


typedef enum {


    // Page 0: Command and status
    CommandReg                      = 0x01 << 1,    // starts and stops command execution
    ComIEnReg                       = 0x02 << 1,    // enable and disable interrupt request control bits
    DivIEnReg                       = 0x03 << 1,    // enable and disable interrupt request control bits
    ComIrqReg                       = 0x04 << 1,    // interrupt request bits
    DivIrqReg                       = 0x05 << 1,    // interrupt request bits
    ErrorReg                        = 0x06 << 1,    // error bits showing the error status of the last command executed
    Status1Reg                      = 0x07 << 1,    // communication status bits
    Status2Reg                      = 0x08 << 1,    // receiver and transmitter status bits
    FIFODataReg                     = 0x09 << 1,    // input and output of 64 byte FIFO buffer
    FIFOLevelReg                    = 0x0A << 1,    // number of bytes stored in the FIFO buffer
    WaterLevelReg                   = 0x0B << 1,    // level for FIFO underflow and overflow warning
    ControlReg                      = 0x0C << 1,    // miscellaneous control registers
    BitFramingReg                   = 0x0D << 1,    // adjustments for bit-oriented frames
    CollReg                         = 0x0E << 1,    // bit position of the first bit-collision detected on the RF interface
    //                                                0x0F                  // reserved for future use


    // Page 1: Command
    
    ModeReg                         = 0x11 << 1,    // defines general modes for transmitting and receiving
    TxModeReg                       = 0x12 << 1,    // defines transmission data rate and framing
    RxModeReg                       = 0x13 << 1,    // defines reception data rate and framing
    TxControlReg                    = 0x14 << 1,    // controls the logical behavior of the antenna driver pins TX1 and TX2
    TxASKReg                        = 0x15 << 1,    // controls the setting of the transmission modulation
    TxSelReg                        = 0x16 << 1,    // selects the internal sources for the antenna driver
    RxSelReg                        = 0x17 << 1,    // selects internal receiver settings
    RxThresholdReg                  = 0x18 << 1,    // selects thresholds for the bit decoder
    DemodReg                        = 0x19 << 1,    // defines demodulator settings
    MfTxReg                         = 0x1C << 1,    // controls some MIFARE communication transmit parameters
    MfRxReg                         = 0x1D << 1,    // controls some MIFARE communication receive parameters
    SerialSpeedReg                  = 0x1F << 1,    // selects the speed of the serial UART interface


    // Page 2: Configuration
    CRCResultRegH                   = 0x21 << 1,    // shows the MSB and LSB values of the CRC calculation
    CRCResultRegL                   = 0x22 << 1,
    ModWidthReg                     = 0x24 << 1,    // controls the ModWidth setting?
    RFCfgReg                        = 0x26 << 1,    // configures the receiver gain
    GsNReg                          = 0x27 << 1,    // selects the conductance of the antenna driver pins TX1 and TX2 for modulation
    CWGsPReg                        = 0x28 << 1,    // defines the conductance of the p-driver output during periods of no modulation
    ModGsPReg                       = 0x29 << 1,    // defines the conductance of the p-driver output during periods of modulation
    TModeReg                        = 0x2A << 1,    // defines settings for the internal timer
    TPrescalerReg                   = 0x2B << 1,    // the lower 8 bits of the TPrescaler value. The 4 high bits are in TModeReg.
    TReloadRegH                     = 0x2C << 1,    // defines the 16-bit timer reload value
    TReloadRegL                     = 0x2D << 1,
    TCounterValueRegH               = 0x2E << 1,    // shows the 16-bit timer value
    TCounterValueRegL               = 0x2F << 1,


    // Page 3: Test Registers
    TestSel1Reg                     = 0x31 << 1,    // general test signal configuration
    TestSel2Reg                     = 0x32 << 1,    // general test signal configuration
    TestPinEnReg                    = 0x33 << 1,    // enables pin output driver on pins D1 to D7
    TestPinValueReg                 = 0x34 << 1,    // defines the values for D1 to D7 when it is used as an I/O bus
    TestBusReg                      = 0x35 << 1,    // shows the status of the internal test bus
    AutoTestReg                     = 0x36 << 1,    // controls the digital self-test
    VersionReg                      = 0x37 << 1,    // shows the software version
    AnalogTestReg                   = 0x38 << 1,    // controls the pins AUX1 and AUX2
    TestDAC1Reg                     = 0x39 << 1,    // defines the test value for TestDAC1
    TestDAC2Reg                     = 0x3A << 1,    // defines the test value for TestDAC2
    TestADCReg                      = 0x3B << 1     // shows the value of ADC I and Q channels

} PCD_Register;


typedef enum {
    STATUS_OK                       ,       // Success
    STATUS_ERROR                    ,       // Error in communication
    STATUS_COLLISION                ,       // Collission detected
    STATUS_TIMEOUT                  ,       // Timeout in communication.
    STATUS_NO_ROOM                  ,       // A buffer is not big enough.
    STATUS_INTERNAL_ERROR           ,       // Internal error in the code. Should not happen ;-)
    STATUS_INVALID                  ,       // Invalid argument.
    STATUS_CRC_WRONG                ,       // The CRC_A does not match
    STATUS_MIFARE_NACK              = 0xff  // A MIFARE PICC responded with NAK.
} StatusCode;


void RC522_WriteRegister(PCD_Register reg , unsigned int value);
void RC522_WritebytesRegister(PCD_Register reg , byte count , byte *values);
void RC522_init(void);
void RC522_Reset(void);
void RC522_AntennaOn(void);
byte RC522_ReadRegister( PCD_Register reg );
void RC522_ReadbytesRegister(PCD_Register reg , byte count , byte *values , byte rxAlign);
byte RC522_IsNewCardPresent(void);
StatusCode PICC_RequestA( byte *bufferATQA,byte *bufferSize);
StatusCode PICC_REQA_or_WUPA( byte command, byte *bufferATQA, byte *bufferSize );
void PCD_ClearRegisterBitMask( PCD_Register reg, byte mask);
StatusCode PCD_TransceiveData( byte *sendData, byte sendLen, byte *backData, byte *backLen, byte *validBits, byte rxAlign, byte checkCRC );
StatusCode PCD_CommunicateWithPICC( byte command, byte waitIRq, byte *sendData, byte sendLen, byte *backData, byte *backLen, byte *validBits, byte rxAlign, byte checkCRC);
void PCD_SetRegisterBitMask( PCD_Register reg, byte mask);
StatusCode PCD_CalculateCRC( byte *data, byte length, byte *result );
byte RC522_ReadCardSerial(void);
StatusCode PICC_Select( Uid *uid, byte validBits );
StatusCode RC522_HaltA(void);
void RC522_StopCrypto1(void);
#endif
