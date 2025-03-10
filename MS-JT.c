#include <c8051F040.h>	// declaratii SFR
#include <wdt.h>
#include <osc.h>
#include <port.h>
#include <uart0.h>
#include <uart1.h>
#include <lcd.h>
#include <keyb.h>
#include <Protocol.h>
#include <UserIO.h>

nod retea[NR_NODURI];				// retea cu 3 noduri

unsigned char STARE_COM = 0;		// starea initiala a FSA COM
unsigned char STARE_IO 	= 0;		// stare initiala FSA interfata IO - asteptare comenzi
unsigned char TIP_NOD	= 0;		// tip nod initial: Slave sau No JET
unsigned char ADR_MASTER;			// adresa nod master - numai MS

extern unsigned char AFISARE;		// flag permitere afisare

//***********************************************************************************************************
void TxMesaj(unsigned char i);				// transmisie mesaj destinat nodului i
unsigned char RxMesaj(unsigned char i);		// primire mesaj de la nodul i

//***********************************************************************************************************
void main (void) {
	unsigned char i, found;				// variabile locale
	
	WDT_Disable();							// dezactiveaza WDT
	SYSCLK_Init();							// initializeaza si selecteaza oscilatorul ales in osc.h
	UART1_Init(NINE_BIT, BAUDRATE_COM);		// initilizare UART1 - port comunicatie (TxD la P1.0 si RxD la P1.1)
	UART1_TxRxEN (1,1);						// validare Tx si Rx UART1
	
 	PORT_Init ();							// conecteaza perifericele la pini (UART0, UART1) si stabileste tipul pinilor

	LCD_Init();    							// 2 linii, display ON, cursor OFF, pozitia initiala (0,0)
	KEYB_Init();							// initializare driver tastatura matriciala locala
	UART0_Init(EIGHT_BIT, BAUDRATE_IO);		// initializare UART0  - conectata la USB-UART (P0.0 si P0.1)

	Timer0_Init();  						// initializare Timer 0

 	EA = 1;                         		// valideaza intreruperile
 	SFRPAGE = LEGACY_PAGE;          		// selecteaza pagina 0 SFR
	
	for(i = 0; i < NR_NODURI; i++){		// initializare structuri de date
		retea[i].full = 0;					// initializeaza buffer gol pentru toate nodurile
		retea[i].bufasc[0] = ':';			// pune primul caracter in buffer-ele ASCII ':'
	}

	Afisare_meniu();			   			// Afiseaza meniul de comenzi
	
 	while(1){								// bucla infinita
																
		switch(STARE_COM){
			case 0:

#if(PROTOCOL == MS)							// nodul este slave, asteapta mesaj complet si corect de la master	

				switch(RxMesaj(ADR_NOD)){			// asteapta un mesaj de la master
					case TMO:							
						
														// anunta ca nodul curent devine master	???
						TIP_NOD=MASTER;								// nodul curent devine master
						Afisare_meniu();								// Afiseaza meniul de comenzi
						STARE_COM=2;								// trece in starea 2
														// primul slave va fi cel care urmeaza dupa noul master	????
								break;

					case ROK:									
						Afisare_mesaj();
						break;	// a primit un mesaj de la master, il afiseaza
					case POK:	
						STARE_COM = 1; 					
						break;	// si trebuie sa raspunda
					case CAN:
						Error("Mesaj incomplet");								
						break;	// afiseaza eroare Mesaj incomplet
					case TIP:									
						Error("Tip mesaj necunoscut");
						break;	// afiseaza eroare Tip mesaj necunoscut
					case ESC:
						Error("Eroare SC");									
						break;	// afiseaza Eroare SC
					default:
						Error("cod UNK");
						Delay(1000);
						break;	// afiseaza cod eroare necunoscut, asteapta 1 sec
				}
#endif

#if(PROTOCOL == JT)									// nodul nu detine jetonul, asteapta un mesaj util sau jetonul
				
				switch(RxMesaj(ADR_NOD)){				// asteapta jetonul de la master
					
					case TMO:								// a primit un mesaj USER_MES
																// anunta ca nodul a regenerat jetonul 
																// nodul curent detine jetonul
																// Daca e permisa afisarea, afiseaza meniul de comenzi
																// trece in starea 1
						break;

					
					case JOK:								// a primit jetonul
						
						Delay(WAIT/2);						// asteapta WAIT/2 ms
					
																						// adresa HW dest este adr_hw_src
																						// adresa HW src este ADR_NOD
																						// tip mesaj = JET_MES
																						// transmite mesajul JET_MES din bufferul ADR_NOD
																						// nodul curent detine jetonul
  
				}
#endif								
				break;

			case 1:											

#if(PROTOCOL == MS)										// nodul este slave, transmite mesaj catre master			
																
																		// cauta sa gaseasca un mesaj util de transmis	 ????
												
																
																
															
														
				if(found)												 	// daca gaseste un nod i
				{
				retea[i].bufbin.adresa_hw_dest=ADR_MASTER;					// adresa HW dest este ADR_MASTER
				TxMesaj(i);											// transmite mesajul pentru nodul i				
				}													
				else
				{
																		// daca nu gaseste, construieste un mesaj in bufferul ADR_MASTER
					retea[ADR_MASTER].bufbin.adresa_hw_dest=ADR_MASTER;			// adresa HW dest este ADR_MASTER
					retea[ADR_MASTER].bufbin.adresa_hw_src=ADR_NOD;				// adresa HW src este ADR_NOD
					retea[ADR_MASTER].bufbin.tipmes= POLL_MES;						// tip mesaj = POLL_MES
					TxMesaj(ADR_MASTER);									// transmite mesajul din bufferul ADR_MASTER
				}
																
																		
															
					STARE_COM=0;											// trece in starea 0, sa astepte un nou mesaj de la master
				break;
#endif

#if(PROTOCOL == JT)							// nodul detine jetonul, poate transmite un mesaj USER_MES				

																						// asteapta procesarea mesajului la destinatie (WAIT/2 msec)
#endif	
				
			break;	
				
			case 2:
															// tratare stare 2 si comutare stare

#if(PROTOCOL == MS)											// nodul este master, tratare stare 2 si comutare stare
				if(++i==ADR_NOD)						   // selecteaza urmatorul slave (incrementeaza i si sare peste adresa proprie)
					i++;
				else if(i>NR_NODURI)
					i=0;																
																	

				retea[ADR_NOD].bufbin.adresa_hw_dest=i;													// adresa HW dest este i
				if(1)
				{
																  // daca in bufferul i se afla un mesaj util, il transmite	 ????
				}
				else
				{
																  // altfel, construieste un mesaj de interogare in bufferul i
					retea[ADR_NOD].bufbin.adresa_hw_src=ADR_NOD;			// adresa HW src este ADR_NOD
					retea[ADR_NOD].bufbin.tipmes=POLL_MES;					// tip mesaj = POLL_MES
					TxMesaj(i);										// transmite mesajul din bufferul i
				}													
																	
				STARE_COM=3;										// trece in starea 3, sa astepte raspunsul de la slave-ul i

#endif

#if(PROTOCOL == JT)											// nodul transmite jetonul urmatorului nod 		
																				// asteapta WAIT/2 sec
																				// trece in starea 3, sa astepte confirmarea de la nodul i ca jetonul a fost primit
#endif

			break;

			case 3:

#if(PROTOCOL == MS)										// nodul este master, asteapta mesaj de la slave	
																
				switch(RxMesaj(i)){								// asteapta un raspuns de la slave i
					case TMO:										// afiseaza eroare Timeout nod i
								
								break;
					case ROK:													break;	// a primit un mesaj de date, il afiseaza
																						// a primit un mesaj de interogare, trece mai departe
					case ERI:													break;	// afiseaza Eroare incadrare
					case ERA:													break;	// afiseaza Eroare adresa
					case TIP:													break;	// afiseaza Tip mesaj necunoscut
					case OVR:													break;	// afiseaza Eroare suprapunere
					case ESC:													break;	// afiseaza Eroare SC
					case CAN:													break;	// afiseaza mesaj incomplet

					default:													break;	// afiseaza Eroare necunoscuta, apoi asteapta 1000ms
				}
																	// revine in starea 2 (a primit sau nu un raspuns de la slave, trece la urmatorul slave)
#endif

			
#if(PROTOCOL == JT)												// asteapta confirmarea primirii jetonului de care nodul i		
				switch(RxMesaj(i)){									// asteapta un raspuns de la nod i
					case TMO:																// afiseaza eroare Timeout nod i
								

																							// revine in starea 2 (nu a primit raspuns)
								break;
					case JOK:																// a primit confirmarea transferului jetonului, revine in starea 0
																									// nodul nu mai detine jetonul
																									// daca afisarea e permisa, afiseaza meniul
								break;
					case ERI:																// afiseaza Eroare incadrare
																									// revine in starea 0
																									// nodul nu mai detine jetonul
																									// afiseaza meniul
										break;			
					case ERA:																// afiseaza Eroare adresa
																									// revine in starea 0
																									// nodul nu mai detine jetonul
																									// afiseaza meniul
										break;			
					
					case CAN:																// afiseaza Mesaj incomplet
																									// revine in starea 0
																									// nodul nu mai detine jetonul
																									// afiseaza meniul
										break;			
					
					case TIP:																// afiseaza Tip mesaj necunoscut
																									// revine in starea 0
																									// nodul nu mai detine jetonul
																									// afiseaza meniul
										break;			
					
					case ESC:																// afiseaza Eroare SC
																									// revine in starea 0
																									// nodul nu mai detine jetonul
																									// afiseaza meniul
										break;			

					default:																// afiseaza Eroare necunoscuta
																									// asteapta 1000 ms
																									// revine in starea 0
																									// nodul nu mai detine jetonul
																									// afiseaza meniul
										break;			
			}
				
#endif
			
			break;			
		}
		
		UserIO();							// apel functie interfata
	}
}
