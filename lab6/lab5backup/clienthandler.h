/* 
 * handles a connection from a client
 *
 * precondition: clientfd is a valid connection from a client
 * postcondition: clientfd is closed
 */
void handleconnection(const int clientfd, char* user_agent); 

