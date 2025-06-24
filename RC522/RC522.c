#include<LPC21XX.H>
#include"spi.h"
#include"delay.h"
#include"RC522.h"
Uid uid;

void RC522_WriteRegister(PCD_Register reg , unsigned int value)
{
    IOCLR0 |= cs_pin;                               // Select slave
    spi0_tx(reg);                                   // Send register address
    spi0_tx(value);                                 // Send value
    IOSET0 |= cs_pin;                               // Release slave
}

void RC522_WritebytesRegister(PCD_Register reg , byte count , byte *values)
{
    byte index;
    IOCLR0 |= cs_pin;                               // Select slave
    spi0_tx(reg);                                   // Send register address
    for( index=0 ; index < count ; index++)
    {
        spi0_tx(values[index]);                     // Send values
    }
    IOSET0 |= cs_pin;                               // Release slave
}


void RC522_init(void)
{
    byte hardReset = _false ;                           // Set the chipSelectPin as digital output, do not select the slave ye
    IODIR0 &= ~ _resetPowerDownPin;
    if( ((IOPIN0 >> _resetPin) & 1) == 0 )              // The MFRC522 chip is in power down mode.
    {
        IODIR0 |=  _resetPowerDownPin;                  // Now set the resetPowerDownPin as digital output.
        IOCLR0 |=  _resetPowerDownPin;                  // Make sure we have a clean LOW state.
        delay_ms(2);
        IOSET0 |=  _resetPowerDownPin;                  // Exit power down mode. This triggers a hard reset.
        delay_ms(50);
        hardReset = _true ;
    }

    if (!hardReset) {
        RC522_Reset();
    }

    // Reset baud rates
    RC522_WriteRegister(TxModeReg, 0x00);
    RC522_WriteRegister(RxModeReg, 0x00);           // Reset ModWidthReg
    RC522_WriteRegister(ModWidthReg, 0x26);         // When communicating with a PICC we need a timeout if something goes wrong.
    RC522_WriteRegister(TModeReg, 0x80);            // TAuto=1; timer starts automatically at the end of the transmission in all communication modes at all speeds
    RC522_WriteRegister(TPrescalerReg, 0xA9);       // TPreScaler = TModeReg[3..0]:TPrescalerReg, ie 0x0A9 = 169 => f_timer=40kHz, ie a timer period of 25s.
    RC522_WriteRegister(TReloadRegH, 0x03);         // Reload timer with 0x3E8 = 1000, ie 25ms before timeout.
    RC522_WriteRegister(TReloadRegL, 0xE8);
    RC522_WriteRegister(TxASKReg, 0x40);            // Default 0x00. Force a 100 % ASK modulation independent of the ModGsPReg register setting
    RC522_WriteRegister(ModeReg, 0x3D);             // Default 0x3F. Set the preset value for the CRC coprocessor for the CalcCRC command to 0x6363 (ISO 14443-3 part 6.2.4)
    RC522_AntennaOn();                              // Enable the antenna driver pins TX1 and TX2 (they were disabled by the reset)
}


void RC522_Reset(void)
{
    unsigned char count = 0;
    RC522_WriteRegister(CommandReg, PCD_SoftReset);
    do {
        delay_ms(50);
    } while ((RC522_ReadRegister(CommandReg) & (1 << 4)) && (++count) < 3);
}

void RC522_AntennaOn(void)
{
    unsigned char value = RC522_ReadRegister(TxControlReg);
    if ((value & 0x03) != 0x03) {
        RC522_WriteRegister(TxControlReg, value | 0x03);
    }
}

byte RC522_ReadRegister( PCD_Register reg )
{
    byte value;
    IOCLR0 |= cs_pin;                               // Select slave
    spi0_tx(0x80 | reg);                            // Send register address with msb 1 to read.
    value = spi0_tx(0);                             // Send dummy value to recieve value
    IOSET0 |= cs_pin;                               // Release slave
    return value;
}
void RC522_ReadbytesRegister(PCD_Register reg , byte count , byte *values , byte rxAlign)
{
    byte address = 0x80 | reg;                                     // MSB == 1 is for reading. LSB is not used in address. Datasheet section 8.1.2.3.
    byte index = 0;                                                // Index in values array.
    byte mask=0;
    byte value=0;
    if (count == 0) {
        return;
    }
    IOCLR0 |= cs_pin;                                              // Select slave
    count--;
    spi0_tx(address);
    if (rxAlign)
    {
        mask = (0xFF << rxAlign) & 0xFF;
        value = spi0_tx(address);
        values[0] = (values[0] & ~mask) | (value & mask);
        index++;
    }
    while (index < count) {
        values[index] = spi0_tx(address);                       // Read value and tell that we want to read the same address again.
        index++;
    }
    values[index] = spi0_tx(0);
    IOSET0 |= cs_pin;                                           // Release slave
}

byte RC522_IsNewCardPresent(void)
{
    StatusCode result;
    byte bufferATQA[2];
    byte bufferSize = sizeof(bufferATQA);
    RC522_WriteRegister(TxModeReg, 0x00);           // Reset baud rates
    RC522_WriteRegister(RxModeReg, 0x00);
    RC522_WriteRegister(ModWidthReg, 0x26);         // Reset ModWidthReg
    result = PICC_RequestA(bufferATQA, &bufferSize);
    return (result == STATUS_OK || result == STATUS_COLLISION);
}

StatusCode PICC_RequestA( byte *bufferATQA,byte *bufferSize)
{
    return PICC_REQA_or_WUPA(PICC_CMD_REQA, bufferATQA, bufferSize);
}

StatusCode PICC_REQA_or_WUPA( byte command, byte *bufferATQA, byte *bufferSize )
{
    byte validBits;
    StatusCode status;
    if (bufferATQA == 0 || *bufferSize < 2) {                // The ATQA response is 2 bytes long.
        return STATUS_NO_ROOM;
    }
    PCD_ClearRegisterBitMask(CollReg, 0x80);                // ValuesAfterColl=1 => Bits received after collision are cleared.
    validBits = 7;                                          // For REQA and WUPA we need the short frame format - transmit only 7 bits of the last (and only) byte. TxLastBits = BitFramingReg[2..0]
    status = PCD_TransceiveData(&command, 1, bufferATQA, bufferSize, &validBits, 0 , 0 );
    if (status != STATUS_OK) {
        return status;
    }
    if (*bufferSize != 2 || validBits != 0) {               // ATQA must be exactly 16 bits.
        return STATUS_ERROR;
    }
    return STATUS_OK;
}
void PCD_ClearRegisterBitMask( PCD_Register reg, byte mask)
{
    byte tmp;
    tmp = RC522_ReadRegister(reg);
    RC522_WriteRegister(reg, tmp & (~mask));
}


StatusCode PCD_TransceiveData( byte *sendData, byte sendLen, byte *backData, byte *backLen, byte *validBits, byte rxAlign, byte checkCRC )
{
    byte waitIRq = 0x30;            // RxIRq and IdleIRq
    return PCD_CommunicateWithPICC(PCD_Transceive, waitIRq, sendData, sendLen, backData, backLen, validBits, rxAlign, checkCRC);
}

StatusCode PCD_CommunicateWithPICC( byte command, byte waitIRq, byte *sendData, byte sendLen, byte *backData, byte *backLen, byte *validBits, byte rxAlign, byte checkCRC)
{
    StatusCode status;
    byte txLastBits;
    byte bitFraming;
    unsigned int deadline = 36;
    byte completed = 0;
    byte n;
    byte errorRegValue;
    byte _validBits = 0;
    byte controlBuffer[2];
    txLastBits = validBits ? *validBits : 0;
    bitFraming = (rxAlign << 4) + txLastBits;


    RC522_WriteRegister(CommandReg, PCD_Idle);                               // Stop any active command.
    RC522_WriteRegister(ComIrqReg, 0x7F);                                    // Clear all seven interrupt request bits
    RC522_WriteRegister(FIFOLevelReg, 0x80);                                 // FlushBuffer = 1, FIFO initialization
    RC522_WritebytesRegister(FIFODataReg, sendLen, sendData);                // Write sendData to the FIFO
    RC522_WriteRegister(BitFramingReg, bitFraming);                          // Bit adjustments
    RC522_WriteRegister(CommandReg, command);                                // Execute the command
    if (command == PCD_Transceive) {
        PCD_SetRegisterBitMask(BitFramingReg, 0x80);                         // StartSend=1, transmission of data starts
    }
    deadline = 36;
    completed = 0;
    T0PC = 0;
    T0PR = 15000-1;
    T0TC = 0;
    T0TCR = 1;
    do {
        n = RC522_ReadRegister(ComIrqReg);
        if (n & waitIRq) {                                                  // One of the interrupts that signal success has been set.
            completed = 1;
            break;
        }
        if (n & 0x01) {                                                     // Timer interrupt - nothing received in 25ms
            return STATUS_TIMEOUT;
        }
    } while( T0TC < deadline);
    T0TCR = 0;
    if (!completed) {
        return STATUS_TIMEOUT;
    }
    errorRegValue = RC522_ReadRegister(ErrorReg);
    if (errorRegValue & 0x13) {                                             // BufferOvfl ParityErr ProtocolErr
        return STATUS_ERROR;
    }
    if (backData && backLen) {
        n = RC522_ReadRegister(FIFOLevelReg);                           // Number of bytes in the FIFO
        if (n > *backLen) {
            return STATUS_NO_ROOM;
        }
        *backLen = n;                                                                                   // Number of bytes returned
        RC522_ReadbytesRegister(FIFODataReg, n, backData, rxAlign);     // Get received data from FIFO
        _validBits = RC522_ReadRegister(ControlReg) & 0x07;             // RxLastBits[2:0] indicates the number of valid bits in the last received byte. If this value is 000b, the whole byte is valid.
        if (validBits) {
            *validBits = _validBits;
        }
    }
    // Tell about collisions
    if (errorRegValue & 0x08) {             // CollErr
        return STATUS_COLLISION;
    }

    // Perform CRC_A validation if requested.
    if (backData && backLen && checkCRC) {
        
        if (*backLen == 1 && _validBits == 4) {
            return STATUS_MIFARE_NACK;
        }
        
        if (*backLen < 2 || _validBits != 0) {
            return STATUS_CRC_WRONG;
        }
        status = PCD_CalculateCRC(&backData[0], *backLen - 2, &controlBuffer[0]);
        if (status != STATUS_OK) {
            return status;
        }
        if ((backData[*backLen - 2] != controlBuffer[0]) || (backData[*backLen - 1] != controlBuffer[1])) {
            return STATUS_CRC_WRONG;
        }
    }
    return STATUS_OK;
}
void PCD_SetRegisterBitMask( PCD_Register reg, byte mask)
{
    byte tmp;
    tmp = RC522_ReadRegister(reg);
    RC522_WriteRegister(reg, tmp | mask);
}
StatusCode PCD_CalculateCRC( byte *data, byte length, byte *result ) {


    unsigned int deadline = 89;
    byte n ;
    RC522_WriteRegister(CommandReg, PCD_Idle);                      // Stop any active command.
    RC522_WriteRegister(DivIrqReg, 0x04);                           // Clear the CRCIRq interrupt request bit
    RC522_WriteRegister(FIFOLevelReg, 0x80);                        // FlushBuffer = 1, FIFO initialization
    RC522_WritebytesRegister(FIFODataReg, length, data);            // Write data to the FIFO
    RC522_WriteRegister(CommandReg, PCD_CalcCRC);                   // Start the calculation

    T0PC = 0;
    T0PR = 15000-1;
    T0TC = 0;
    T0TCR = 1;
    do {
        n = RC522_ReadRegister(DivIrqReg);
        if (n & 0x04) {                                              // CRCIRq bit set - calculation done
            RC522_WriteRegister(CommandReg, PCD_Idle);               // Stop calculating CRC for new content in the FIFO.
            // Transfer the result from the registers to the result buffer
            result[0] = RC522_ReadRegister(CRCResultRegL);
            result[1] = RC522_ReadRegister(CRCResultRegH);
            return STATUS_OK;
        }
    } while( T0TC < deadline);
    T0TCR = 0;
    // 89ms passed and nothing happened. Communication with the MFRC522 might be down.
    return STATUS_TIMEOUT;
}

byte RC522_ReadCardSerial(void)
{
    StatusCode result;
    result = PICC_Select(&uid,0);
    return (result == STATUS_OK);
}

StatusCode PICC_Select( Uid *uid, byte validBits )
{
    byte uidComplete;
    byte selectDone;
    byte useCascadeTag;
    byte cascadeLevel = 1;
    StatusCode result;
    byte count;
    byte bytesToCopy;
    byte collisionPos;
    byte checkBit;
    byte index;
    byte valueOfCollReg;
    byte maxBytes;
    byte uidIndex;                                  // The first index in uid->uidByte[] that is used in the current Cascade Level.
    int currentLevelKnownBits;                      // The number of known UID bits in the current Cascade Level.
    byte buffer[9];                                 // The SELECT/ANTICOLLISION commands uses a 7 byte standard frame + 2 bytes CRC_A
    byte bufferUsed;                                // The number of bytes used in the buffer, ie the number of bytes to transfer to the FIFO.
    byte rxAlign;                                   // Used in BitFramingReg. Defines the bit position for the first bit received.
    byte txLastBits;                                // Used in BitFramingReg. The number of valid bits in the last transmitted byte.
    byte *responseBuffer;
    byte responseLength;
    if (validBits > 80) {
        return STATUS_INVALID;
    }
    PCD_ClearRegisterBitMask(CollReg, 0x80);
    uidComplete = 0;
    
    while (!uidComplete) {

        switch (cascadeLevel) {
            case 1:
                buffer[0] = PICC_CMD_SEL_CL1;
                uidIndex = 0;
                useCascadeTag = validBits && uid->size > 4;     // When we know that the UID has more than 4 bytes
                break;
            case 2:
                buffer[0] = PICC_CMD_SEL_CL2;
                uidIndex = 3;
                useCascadeTag = validBits && uid->size > 7;     // When we know that the UID has more than 7 bytes
                break;
            case 3:
                buffer[0] = PICC_CMD_SEL_CL3;
                uidIndex = 6;
                useCascadeTag = 0;                              // Never used in CL3.
                break;
            default:
                return STATUS_INTERNAL_ERROR;
        }

        // How many UID bits are known in this Cascade Level?

        currentLevelKnownBits = validBits - (8 * uidIndex);
        if (currentLevelKnownBits < 0) {
            currentLevelKnownBits = 0;
        }

        // Copy the known bits from uid->uidByte[] to buffer[]

        index = 2;                                                  // destination index in buffer[]
        if (useCascadeTag) {
            buffer[index++] = PICC_CMD_CT;
        }
        bytesToCopy = currentLevelKnownBits / 8 + (currentLevelKnownBits % 8 ? 1 : 0);
        if (bytesToCopy) {
            maxBytes = useCascadeTag ? 3 : 4;                       // Max 4 bytes in each Cascade Level. Only 3 left if we use the Cascade Tag
            if (bytesToCopy > maxBytes) {
                bytesToCopy = maxBytes;
            }
            for (count = 0; count < bytesToCopy; count++) {
                buffer[index++] = uid->uidByte[uidIndex + count];
            }
        }
        if (useCascadeTag) {
            currentLevelKnownBits += 8;
        }
        selectDone = 0;
        while (!selectDone) {

            if (currentLevelKnownBits >= 32) {
                buffer[1] = 0x70;
                buffer[6] = buffer[2] ^ buffer[3] ^ buffer[4] ^ buffer[5];
                result = PCD_CalculateCRC(buffer, 7, &buffer[7]);
                if (result != STATUS_OK) {
                        return result;
                }
                txLastBits              = 0;                    // 0 => All 8 bits are valid.
                bufferUsed              = 9;
                responseBuffer  = &buffer[6];                   // Store response in the last 3 bytes of buffer (BCC and CRC_A - not needed after tx)
                responseLength  = 3;
            }
            else { // This is an ANTICOLLISION.
                txLastBits              = currentLevelKnownBits % 8;
                count                   = currentLevelKnownBits / 8;        // Number of whole bytes in the UID part.
                index                   = 2 + count;                                    // Number of whole bytes: SEL + NVB + UIDs
                buffer[1]               = (index << 4) + txLastBits;        // NVB - Number of Valid Bits
                bufferUsed              = index + (txLastBits ? 1 : 0);
                responseBuffer  = &buffer[index];
                responseLength  = sizeof(buffer) - index;                   // Store response in the unused part of buffer
            }
            rxAlign = txLastBits;                                           // Set bit adjustments
            // Having a separate variable is overkill. But it makes the next line easier to read.
            RC522_WriteRegister(BitFramingReg, (rxAlign << 4) + txLastBits);        // RxAlign = BitFramingReg[6..4]. TxLastBits = BitFramingReg[2..0]
            //completed up to here.
            // Transmit the buffer and receive the response.


            result = PCD_TransceiveData(buffer, bufferUsed, responseBuffer, &responseLength, &txLastBits, rxAlign,0);
            if (result == STATUS_COLLISION) {                                       // More than one PICC in the field => collision.
                valueOfCollReg = RC522_ReadRegister(CollReg);                       // CollReg[7..0] bits are: ValuesAfterColl reserved CollPosNotValid CollPos[4:0]
                if (valueOfCollReg & 0x20) {                                        // CollPosNotValid
                    return STATUS_COLLISION;                                        // Without a valid collision position we cannot continue
                }
                collisionPos = valueOfCollReg & 0x1F;                               // Values 0-31, 0 means bit 32.
                if (collisionPos == 0) {
                    collisionPos = 32;
                }
                if (collisionPos <= currentLevelKnownBits) {                        // No progress - should not happen
                    return STATUS_INTERNAL_ERROR;
                }
                // Choose the PICC with the bit set.
                currentLevelKnownBits   = collisionPos;
                count                   = currentLevelKnownBits % 8;                // The bit to modify
                checkBit                = (currentLevelKnownBits - 1) % 8;
                index                   = 1 + (currentLevelKnownBits / 8) + (count ? 1 : 0); // First byte is index 0.
                buffer[index]          |= (1 << checkBit);
            }
            else if (result != STATUS_OK) {
                return result;
            }
            else {          // STATUS_OK
                if (currentLevelKnownBits >= 32) {           // This was a SELECT.
                    selectDone = 1;                          // No more anticollision
                    // We continue below outside the while.
                }
                else { // This was an ANTICOLLISION.
                    // We now have all 32 bits of the UID in this Cascade Level
                    currentLevelKnownBits = 32;
                    // Run loop again to do the SELECT.
                }
            }
        } // End of while (!selectDone)


        // We do not check the CBB - it was constructed by us above.


        // Copy the found UID bytes from buffer[] to uid->uidByte[]
        index                   = (buffer[2] == PICC_CMD_CT) ? 3 : 2;
        bytesToCopy             = (buffer[2] == PICC_CMD_CT) ? 3 : 4;
        for (count = 0; count < bytesToCopy; count++) {
            uid->uidByte[uidIndex + count] = buffer[index++];
        }

        // Check response SAK (Select Acknowledge)
        if (responseLength != 3 || txLastBits != 0) { // SAK must be exactly 24 bits (1 byte + CRC_A).
            return STATUS_ERROR;
        }

        // Verify CRC_A - do our own calculation and store the control in buffer[2..3] - those bytes are not needed anymore.
        result = PCD_CalculateCRC(responseBuffer, 1, &buffer[2]);
        if (result != STATUS_OK) {
            return result;
        }
        if ((buffer[2] != responseBuffer[1]) || (buffer[3] != responseBuffer[2])) {
            return STATUS_CRC_WRONG;
        }
        if (responseBuffer[0] & 0x04) { // Cascade bit set - UID not complete yes
            cascadeLevel++;
        }
        else {
            uidComplete = 1;
            uid->sak = responseBuffer[0];
        }
    } // End of while (!uidComplete)

    // Set correct uid->size
    uid->size = 3 * cascadeLevel + 1;

    return STATUS_OK;
} // End PICC_Select()

StatusCode RC522_HaltA(void)
{
    StatusCode result;
    byte buffer[4];
    // Build command buffer
    buffer[0] = PICC_CMD_HLTA;
    buffer[1] = 0;
    result = PCD_CalculateCRC(buffer, 2, &buffer[2]);
    if (result != STATUS_OK) {
        return result;
    }
    result = PCD_TransceiveData(buffer, sizeof(buffer), 0 , 0 , 0 , 0 , 0 );
    if (result == STATUS_TIMEOUT) {
        return STATUS_OK;
    }
    if (result == STATUS_OK) { // That is ironically NOT ok in this case ;-)
        return STATUS_ERROR;
    }
    return result;
}

void RC522_StopCrypto1(void)
{
    PCD_ClearRegisterBitMask(Status2Reg, 0x08);
}
