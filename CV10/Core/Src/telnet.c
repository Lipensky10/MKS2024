#include "lwip/opt.h"

#if LWIP_NETCONN

#include "lwip/sys.h"
#include "lwip/api.h"
#include <string.h>  // For strtok_r

#define telnet_THREAD_PRIO  ( tskIDLE_PRIORITY + 4 )
#define CMD_BUFFER_LEN 512
char S_buffer[512];

static void http_client(char *s, uint16_t size)
{
	struct netconn *client;
	struct netbuf *buf;
	ip_addr_t ip;
	uint16_t len = 0;
	IP_ADDR4(&ip, 147,229,144,124);
	const char *request = "GET /ip.php HTTP/1.1\r\n"
			"Host: www.urel.feec.vutbr.cz\r\n"
			"Connection: close\r\n"
			"\r\n\r\n";
	client = netconn_new(NETCONN_TCP);
	if (netconn_connect(client, &ip, 80) == ERR_OK) {
		netconn_write(client, request, strlen(request), NETCONN_COPY);
		// Receive the HTTP response
		s[0] = 0;
		while (len < size && netconn_recv(client, &buf) == ERR_OK) {
			len += netbuf_copy(buf, &s[len], size-len);
			s[len] = 0;
			netbuf_delete(buf);
		}
	} else {
		sprintf(s, "Connection error\n");
	}
	netconn_delete(client);
}

// Modified version of the command processor using strtok_r (reentrant)
static void telnet_process_command(char *cmd, struct netconn *conn){
	char *token;
	char *saveptr;  // To store the state of the strtok_r

	// Use strtok_r for reentrancy (thread-safe)
	token = strtok_r(cmd, " ", &saveptr);

	if (token != NULL) {
		if (strcasecmp(token, "LED1") == 0) {
			token = strtok_r(NULL, " ", &saveptr);  // Get the next token
			if (strcasecmp(token, "ON") == 0) {
				HAL_GPIO_WritePin(LED1_GPIO_Port, LED1_Pin, 1);
				sprintf(S_buffer, "LED1 ON\n");
			}
			else if (strcasecmp(token, "OFF") == 0) {
				HAL_GPIO_WritePin(LED1_GPIO_Port, LED1_Pin, 0);
				sprintf(S_buffer, "LED1 OFF\n");
			} else {
				sprintf(S_buffer, "Invalid command for LED1\n");
			}
		}
		else if (strcasecmp(token, "LED2") == 0) {
			token = strtok_r(NULL, " ", &saveptr);
			if (strcasecmp(token, "ON") == 0) {
				HAL_GPIO_WritePin(LED2_GPIO_Port, LED2_Pin, 1);
				sprintf(S_buffer, "LED2 ON\n");
			}
			else if (strcasecmp(token, "OFF") == 0) {
				HAL_GPIO_WritePin(LED2_GPIO_Port, LED2_Pin, 0);
				sprintf(S_buffer, "LED2 OFF\n");
			} else {
				sprintf(S_buffer, "Invalid command for LED2\n");
			}
		}
		else if (strcasecmp(token, "status") == 0) {
			uint32_t status_LED1 = HAL_GPIO_ReadPin(LED1_GPIO_Port, LED1_Pin);
			uint32_t status_LED2 = HAL_GPIO_ReadPin(LED2_GPIO_Port, LED2_Pin);
			sprintf(S_buffer, "LEDs status: LED1 is %s, LED2 is %s\n",
					status_LED1 ? "ON" : "OFF", status_LED2 ? "ON" : "OFF");
		}
		else if (strcasecmp(token, "client") == 0) {
			http_client(S_buffer, sizeof(S_buffer));

		}
		else {
			sprintf(S_buffer, "Unknown command\n");

		}
		netconn_write(conn, S_buffer, strlen(S_buffer), NETCONN_COPY);
	}
}

/*-----------------------------------------------------------------------------------*/
static void telnet_byte_available(uint8_t c, struct netconn *conn)
{
	static uint16_t cnt;
	static char data[CMD_BUFFER_LEN];
	if (cnt < CMD_BUFFER_LEN && c >= 32 && c <= 127) {
		data[cnt++] = c;
	}
	if (c == '\n' || c == '\r') {
		data[cnt] = '\0';
		telnet_process_command(data, conn);  // Process the command when line ends
		cnt = 0;
	}
}
static void telnet_thread(void *arg)
{
	struct netconn *conn, *newconn;
	err_t err, accept_err;
	struct netbuf *buf;
	uint8_t *data;
	u16_t len;

	LWIP_UNUSED_ARG(arg);
	/* Create a new connection identifier. */
	conn = netconn_new(NETCONN_TCP);

	if (conn != NULL)
	{
		/* Bind connection to well-known port number 23 (Telnet). */
		err = netconn_bind(conn, NULL, 23);

		if (err == ERR_OK)
		{
			/* Tell connection to go into listening mode. */
			netconn_listen(conn);

			while (1)
			{
				/* Grab new connection. */
				accept_err = netconn_accept(conn, &newconn);

				/* Process the new connection. */
				if (accept_err == ERR_OK)
				{
					while (netconn_recv(newconn, &buf) == ERR_OK)
					{
						do
						{
							netbuf_data(buf, (void**)&data, &len);
							while (len--) telnet_byte_available(*data++, newconn);

						} while (netbuf_next(buf) >= 0);

						netbuf_delete(buf);
					}

					/* Close connection and discard connection identifier. */
					netconn_close(newconn);
					netconn_delete(newconn);
				}
			}
		}
	}
	else
	{
		netconn_delete(newconn);
	}
}


/*-----------------------------------------------------------------------------------*/

void telnet_init(void)
{
	sys_thread_new("telnet_thread", telnet_thread, NULL, DEFAULT_THREAD_STACKSIZE, telnet_THREAD_PRIO);
}

#endif /* LWIP_NETCONN */
