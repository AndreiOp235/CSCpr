#include <c8051F040.h> // declaratii SFR
#include <uart1.h>
#include <Protocol.h>
#include <UserIO.h>

extern nod retea[]; // reteaua Master-Slave, cu 3 noduri

extern unsigned char TIP_NOD;	 // tip nod
extern unsigned char ADR_MASTER; // adresa nodului master

extern unsigned char timeout; // variabila globala care indica expirare timp de asteptare eveniment
//***********************************************************************************************************
unsigned char RxMesaj(unsigned char i);		 // primire mesaj de la nodul i
unsigned char ascii2bin(unsigned char *ptr); // functie de conversie 2 caractere ASCII HEX in binar

//***********************************************************************************************************
unsigned char RxMesaj(unsigned char i)
{ // receptie mesaj
	unsigned char j, ch, sc, adresa_hw_dest, adresa_hw_src, screc, src, dest, lng, tipmes, *ptr;
	if (TIP_NOD == MASTER)							// Daca nodul este master (asteapta raspuns de la slave)
	{																		// sau a transmis jetonul si asteapta confirmarea preluarii acestuia - un mesaj JET_MES
		ch = UART1_Getch_TMO(WAIT); 			// M: asteapta cu timeout primul caracter al raspunsului de la slave
		if (timeout)
		{
			return TMO; 										// M: timeout, terminare receptie
		}
		retea[i].full = 0; 								// M: raspunsul de la slave vine, considera ca mesajul anterior a fost transmis cu succes

		if (ch != ':') 										// M: daca primul caracter nu este ':'...
		{			   													// M: ignora restul mesajului
			do
			{
				ch = UART1_Getch_TMO(5);
			} 
			while (!timeout);
			return ERI; 										// M: eroare de incadrare, terminare receptie
		}
		else
		{
			ptr = retea[ADR_NOD].bufasc + 1;// M: initializare pointer in bufferul ASCII

			*ptr++ = UART1_Getch_TMO(5); 		// M: asteapta cu timeout primul caracter ASCII al adresei HW
			if (timeout)
			{
				return CAN; 									// M: timeout, terminare receptie
			}

			*ptr-- = UART1_Getch_TMO(5); 		// M: asteapta cu timeout al doilea caracter al adresei HW
			if (timeout)
			{
				return CAN; 									// M: timeout, terminare receptie
			}
			adresa_hw_dest = ascii2bin(ptr);

			if (adresa_hw_dest == ADR_NOD) // M: daca mesajul nu este pentru acest nod
			{
				ptr++;
			}else{
				do{
					ch = UART1_Getch_TMO(5); 	// M: ignora restul mesajului
				}while (!timeout);
				return ERA; 								// M: adresa HW ASCII gresita, terminare receptie
			}
		}
	}
	else
	{
		do
		{
			do
			{
				ch = UART1_Getch_TMO(2 * WAIT + ADR_NOD * WAIT); // S: asteapta cu timeout primirea primului caracter al unui mesaj de la master
																												// (sau de la nodul care detine jetonul)
				if (timeout)
				{					
					return TMO; // S: timeout, terminare receptie, nodul va deveni master
											// sau va anunta ca s-a pierdut jetonul si va regenera jetonul
				} 
			}while (ch != ':'); // S: asteapta sincronizarea cu inceputul mesajului

			ptr = retea[ADR_NOD].bufasc + 1; // S: initializeaza pointerul in bufferul ASCII
			*ptr++ = UART1_Getch_TMO(5);	 // S: asteapta cu timeout primul caracter ASCII al adresei HW
			if (timeout)
			{
				return CAN; // S: timeout, terminare receptie
			}

			*ptr-- = UART1_Getch_TMO(5); // S: asteapta cu timeout al doilea caracter al adresei HW
			if (timeout)
			{
				return CAN; // S: timeout, terminare receptie
			}

			adresa_hw_dest = ascii2bin(ptr); // S: determina adresa HW destinatie
		}while (adresa_hw_dest != ADR_NOD);		 // S: iese doar cand mesajul era adresat acestui nod
		ptr++;
	}	
	do
	{
		*(++ptr) = UART1_Getch_TMO(5); // M+S: pune in bufasc restul mesajului ASCII HEX
		if (timeout)
		{
			return CAN; // M+S: timeout, terminare receptie
		}
	}while (*ptr != 0x0A); // M+S: reinitializare pointer in bufferul ASCII

	ptr = retea[ADR_NOD].bufasc + 3; 	// M+S: initializeaza screc cu adresa HW dest
	screc = adresa_hw_dest;			 			// M+S: determina adresa HW src

	adresa_hw_src = ascii2bin(ptr); // M+S: aduna adresa HW src
	ptr += 2;
	screc += adresa_hw_src;

	if (TIP_NOD == SLAVE) // Slave actualizeaza adresa Master
	{
		ADR_MASTER = adresa_hw_src;
	}
	tipmes = ascii2bin(ptr); // M+S: determina tipul mesajului
	ptr += 2;
	if (tipmes > 1)
	{
		return TIP; // M+S: cod functie eronat, terminare receptie
	}
	screc += tipmes; // M+S: ia in calcul in screc codul functiei

	if (tipmes == USER_MES) // M+S: Daca mesajul este USER_MES
	{
		src = ascii2bin(ptr); // M+S: determina sursa mesajului
		ptr += 2;
		screc += src;		   // M+S: ia in calcul in screc adresa src
		dest = ascii2bin(ptr); // M+S: determina destinatia mesajului
		ptr += 2;
		screc += dest; // M+S: ia in calcul in screc adresa dest

		if (TIP_NOD == MASTER) // Daca nodul este master...
		{
			if (retea[dest].full == 1) // M: bufferul destinatie este deja plin, terminare receptie
			{
				return OVR;
			}
		}
		lng = ascii2bin(ptr); // M+S: determina lng
		ptr += 2;
		screc += lng; // M+S: ia in calcul in screc lungimea datelor

		if (TIP_NOD == MASTER) // Daca nodul este master...
		{
			retea[dest].bufbin.adresa_hw_src = ADR_NOD; // M: stocheaza in bufbin adresa HW src	egala cu ADR_NOD
			retea[dest].bufbin.tipmes = tipmes;			// M: stocheaza in bufbin tipul mesajului
			retea[dest].bufbin.src = src;				// M: stocheaza in bufbin adresa nodului sursa al mesajului
			retea[dest].bufbin.dest = dest;				// M: stocheaza in bufbin adresa nodului destinatie al mesajului
			retea[dest].bufbin.lng = lng;				// M: stocheaza lng
			
			for (j = 0; j < retea[dest].bufbin.lng; j++)
			{
				retea[dest].bufbin.date[j] = ascii2bin(ptr);
				ptr +=2;
				screc += retea[dest].bufbin.date[j];				
			}
			
			sc = ascii2bin(ptr); 	// M: determina suma de control
			
			if (sc == screc)
			{
				retea[dest].full = 1; // M: mesaj corect, marcare buffer plin
				return ROK;
			}
			else
			{
				return ESC; // M: eroare SC, terminare receptie
			}
		}
		else		// altfel (Daca nodul este slave ...)
		{									 
			retea[ADR_NOD].bufbin.src = src; // S: stocheaza la destsrc codul nodului sursa al mesajului
			retea[ADR_NOD].bufbin.lng = lng; // S: stocheaza lng
			
			for (j = 0; j < retea[ADR_NOD].bufbin.lng; j++)
			{
				retea[ADR_NOD].bufbin.date[j] = ascii2bin(ptr);
				ptr +=2;
				screc += retea[ADR_NOD].bufbin.date[j];				
			}
			sc = ascii2bin(ptr); // S: determina suma de control

			if (sc == screc)
			{							 // daca sc este corecta
				retea[ADR_NOD].full = 1; // S: mesaj corect, marcare buffer plin
				return ROK;
			}
			else		// altfel ...
			{				
				return ESC; // S: eroare SC, terminare receptie
			}
		}
	}
	else			// daca mesajul este POLL_MES sau JET_MES
	{														 
		retea[ADR_NOD].bufbin.adresa_hw_src = adresa_hw_src; // memoreaza adresa hw src pentru a sti de la cine a primit jetonul
		sc = ascii2bin(ptr);								 // M+S: determina suma de control
		if (sc == screc)		// daca sc este corecta
		{				
			return POK; // M+S: returneaza POK sau JOK, au aceeasi valoare
		}
		else		// altfel...
		{				
			return ESC; // M+S: eroare SC, terminare receptie
		}
	}
}

//***********************************************************************************************************
unsigned char ascii2bin(unsigned char *ptr)
{ // converteste doua caractere ASCII HEX de la adresa ptr
	unsigned char bin;

	if (*ptr > '9')
		bin = (*ptr++ - 'A' + 10) << 4; // contributia primului caracter daca este litera
	else
		bin = (*ptr++ - '0') << 4; // contributia primului caracter daca este cifra
	if (*ptr > '9')
		bin += (*ptr++ - 'A' + 10); // contributia celui de-al doilea caracter daca este litera
	else
		bin += (*ptr++ - '0'); // contributia celui de-al doilea caracter daca este cifra
	return bin;
}
