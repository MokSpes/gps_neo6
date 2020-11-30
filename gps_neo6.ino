
/*

    La estructura de los mensajes NMEA es:

    $address{,value}*cs<CR><LF>
	$		: start character : always '$'
	address	: message ID      :
	{,value}: data fields     : each delimited by ',' and variable length
			: end character   : always '*'
	cs		: checksum        : 2 char representing a 8-bit hex number. Exclusive OR off all characters between '$' and '*'


    Interesa el mensaje GPVTG "Course over ground and Ground speed" que da la velocidad en kilometros/hora

    $GPVTG,cogt,T,cogm,M,sog,N,kph,K,mode*cs<CR><LF>
    $GPVTG,77.52,T,,M,0.004,N,0.008,K,A*06

    cogt : degrees     : True course over ground
    T    : Fixed field : denote "True"
	cogm : degrees     : Magnetic course over ground (not output by u-blox NEO6)
	M    : Fixed field : denote "Magnetic"
	sog  : knots       : Speed over ground (in knots)
	N    : Fixed field : denote "Knots"
	kph  : Km/h        : Speed over ground (in Km/h)
	K    : Fixed field : denote "Kilometers per hour"
	mode : character   : fix mode indicator, 'A' for "Autonomous GNSS Fix"
*/


int rx_lon;

void setup()
{
	// initialize serial:
	Serial.begin(9600);

	pinMode(LED_BUILTIN, OUTPUT);
	digitalWrite(LED_BUILTIN, LOW);
}


// automata receptor del mensaje GPS

#define MAX_LINEA 80
unsigned char linea[MAX_LINEA + 1];

void cambia_led()
{
	static bool last_led = 0;

	if (last_led)
	{
		digitalWrite(LED_BUILTIN, LOW);    // turn the LED off by making the voltage LOW
		last_led = 0;
	}
	else
	{
		digitalWrite(LED_BUILTIN, HIGH);
		last_led = 1;
	}
}

/**
 * retorna la velocidad o -1 en caso de error
 */
int16_t buscar_velocidad (char *pl)
{
	uint8_t comas = 0;
	int16_t velocidad = 0;
	char ch;

	// ver si la cabecera del mensaje es la del mensaje NMEA de velocidad (GPVTG)
	if (*pl++ != 'G' || *pl++ != 'P' || *pl++ != 'V' || *pl++ != 'T' || *pl++ != 'G')
		return -1;

	Serial.println(pl);

	// se han de saltar 7 comas
	while (comas != 7)
		if ((ch = *pl++) == '\0') return -1;
		else if (ch == ',') comas++;

	// estoy en el septimo campo, leer la parte entera de la velocidad
	while (true)
		if ((ch = *pl++) >= '0' && ch <= '9') velocidad = velocidad * 10 + (ch - '0');
		else if (ch == '.') return velocidad;
		else return -1;

	cambia_led();
	return -1;
}





bool receptor_nmea(void)
{
	static uint8_t estado = 0; // estado global del receptor
	static uint8_t checksum;
	unsigned char ch;
	bool recibido = false;

	while (Serial.available() > 0)
	{
		ch = Serial.read(); // los errores se descartan en el driver, que la trama sea invalida se ha de detectar con el checksum o el formato

		if (ch == '$') // inicio de trama
		{
			// el '$' no hace falta guardarlo
			estado = 1;
			rx_lon = 0;
			checksum = 0;
		}
		else
			switch (estado)
			{
				case 0:	//esperando inicio de trama
					break;

				case 1:	// se ha recibido '$', recibiendo caracteres
					if (ch == '*') // marca de fin de trama
					{
						linea[rx_lon] = '\0';
						estado = 2;
					}
					else
					{
						checksum ^= ch; // todo lo que va entre '$' y '*' cuenta para el checksum
						if (rx_lon < MAX_LINEA)
							linea[rx_lon++] = ch;
						else
							estado = 0; // linea demasiado larga, abortar recepcion
					}
					break;

				case 2: //se recibió '*' -> éste es es el nibble alto del checksum
					estado = 3;
					break;

				case 3: //se recibió nibble_alto_del_checksum -> éste es el nibble bajo del checcksum
					recibido = true; // marca trama recibida (como no va por interrupciones, no se perderá la trama)
					estado = 0; // preparado para la siguiente
					break;
			}
	}

	return recibido;
}


void loop()
{
	int vel;

	if (receptor_nmea())
	{
		if ((vel = buscar_velocidad(linea)) >= 0) Serial.println(vel);
	}
}
