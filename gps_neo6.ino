int rx_lon;
unsigned char pepe;

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

unsigned char *pe;

int estado = 0, checksum;
int last_led = 0;

void cambia_led()
{
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

void loop()
{
	while (Serial.available() > 0)
	{
		unsigned char ch = Serial.read(); // los errores se descartan en el driver, que la trama sea invalida se ha de detectar con el checksum o el formato
		//		Serial.write((char)ch);

		if (ch == '$') // inicio de trama
		{
			// el '$' no hace falta guardarlo
			estado = 1;
			rx_lon = 0;
			checksum = 0;
		}
		else switch (estado)
		{
		case 0:	//esperando inicio de trama
			break;

		case 1:	// se ha recibido '$', recibiendo caracteres
			//Serial.print((char)ch);
			if (ch == '*') // marca de fin de trama
			{
				linea[rx_lon] = '\0';
				estado = 2;
			}
			else
			{
				checksum ^= ch; // todo lo que va entre '$' y '*' cuenta para el checksum

				if (rx_lon < MAX_LINEA)
				{
					linea[rx_lon++] = ch;
				}
				else
				{
					estado = 0; // linea demasiado larga, abortar recepcion
				}
			}
			break;

		case 2: //se recibió '*' -> éste es es el nibble alto del checksum
			ch -= '0';
			
			estado = 3;
			break;

		case 3: //se recibió nibble_alto_del_checksum -> éste es el nibble bajo del checcksum
			estado = 4;
			cambia_led();
			Serial.println((char *)linea);
			break;

		case 4: //recibida trama
			break;
		}
	}
}
