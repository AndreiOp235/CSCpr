#include <c8051F040.h>	// declaratii SFR
#include <uart1.h>
#include <Protocol.h>
#include <UserIO.h>

extern unsigned char TIP_NOD;			// tip nod initial: Nu Master, Nu Jeton

extern nod retea[];

extern unsigned char timeout;		// variabila globala care indica expirare timp de asteptare eveniment
//***********************************************************************************************************
void TxMesaj(unsigned char i);	// transmisie mesaj destinat nodului i
void bin2ascii(unsigned char ch, unsigned char *ptr);	// functie de conversie octet din binar in ASCII HEX

//***********************************************************************************************************
void TxMesaj(unsigned char i){					// transmite mesajul din buffer-ul i
	unsigned char sc, *ptr, j;
	
	if(retea[i].bufbin.tipmes == POLL_MES)
	{
		sc = retea[i].bufbin.adresa_hw_dest;
		sc += retea[i].bufbin.adresa_hw_src;
		retea[i].bufbin.sc = sc;
	}
	else
	{
		sc = retea[i].bufbin.adresa_hw_dest;
		sc += retea[i].bufbin.adresa_hw_src;
		sc += retea[i].bufbin.tipmes;
		sc += retea[i].bufbin.src;
		sc += retea[i].bufbin.dest;
		sc += retea[i].bufbin.lng;
		
		for(j = 0; j < retea[i].bufbin.lng; ++j) 
		{
			sc += retea[i].bufbin.date[j];
		}
		
		retea[i].bufbin.sc = sc;
	}																			

	ptr = retea[i].bufasc + 1;															
	
	bin2ascii(retea[i].bufbin.adresa_hw_dest, ptr);											
	ptr += 2;
																							
	bin2ascii(retea[i].bufbin.adresa_hw_src, ptr);											
	ptr += 2;
																								
	bin2ascii(retea[i].bufbin.tipmes, ptr); 											
	ptr += 2;
																								
	if(retea[i].bufbin.tipmes == USER_MES)
	{
		bin2ascii(retea[i].bufbin.src, ptr);
		ptr += 2;
		
		bin2ascii(retea[i].bufbin.dest, ptr);
		ptr += 2;
		
		bin2ascii(retea[i].bufbin.lng, ptr);
		ptr += 2;
		
		for(j = 0; j < retea[i].bufbin.lng; ++j) 
		{
			bin2ascii(retea[i].bufbin.date[j], ptr);
			ptr += 2;
		}
	}		
			
	bin2ascii(retea[i].bufbin.sc, ptr);
	ptr += 2;
	
	*ptr++ = 0x0D;
	*ptr++ = 0x0A;
	
	ptr = retea[i].bufasc;
	
	do {
    UART1_PutchPE(*ptr);
	} while (*ptr++ != 0x0A);
	
	if(TIP_NOD != MASTER)
	{
		retea[i].full = 0;
	}
}

//***********************************************************************************************************
void bin2ascii(unsigned char ch, unsigned char *ptr){	// converteste octetul ch in doua caractere ASCII HEX puse la adresa ptr
	unsigned char first, second;
	first = (ch & 0xF0)>>4;						// extrage din ch primul digit
	second = ch & 0x0F;								// extrage din ch al doilea digit
	if(first > 9) *ptr++ = first - 10 + 'A';	// converteste primul digit daca este litera
	else *ptr++ = first + '0';				// converteste primul digit daca este cifra
 	if(second > 9) *ptr++ = second - 10 + 'A';	// converteste al doilea digit daca este litera
	else *ptr++ = second + '0';				// converteste al doilea digit daca este cifra
}
