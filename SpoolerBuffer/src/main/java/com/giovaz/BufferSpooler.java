package com.giovaz;

import java.io.IOException;
import java.net.ServerSocket;
import java.net.Socket;
import java.sql.Connection;
import java.sql.DriverManager;
import java.sql.PreparedStatement;
import java.sql.SQLException;

public class BufferSpooler {
	/**
	 * main program
	 * 
	 * 
	 * run in terminal cmd example
	 * java -jar 8000 jdbc:mariadb://127.0.0.1:3306/MySchema myUser myPwd
	 * 
	 * @param args
	 * 		0 --> server socket listening port
	 * 		1 --> connection url in form jdbc:mariadb://ip:port/schema
	 * 		2 --> db user
	 *		3 --> db pwd 
	 * @throws InterruptedException
	 */
	public static void main(String[] args) throws InterruptedException {
		
		int port = Integer.parseInt(args[0]); 
		String url = args[1];//connection url (jdbc:mariadb://ip:port/schema)
		String user = args[2];//db user 
		String password = args[3];//db pwd

		try (Connection connection = DriverManager.getConnection(url, user, password)) {
			System.out.println("DB connection succeded!");
		} catch (SQLException e) {
			System.err.println("Db connection error: " + e.getMessage());
		}


		try (ServerSocket serverSocket = new ServerSocket(port)) 
		{
			System.out.println("Server socket is up. Awaiting for connection on port " + port + "...");
			while(true)
				try (Socket clientSocket = serverSocket.accept()) 
				{
					System.out.println("["+new java.util.Date()+"]New incoming connection from client: " + clientSocket.getInetAddress());

					/**
					 * *****************************************
					 * The importance of setting a timeout value
					 * ******************************************
					 * once the clientSocket is created, data from the sender should be immediately sent.
					 * In case data does not arrives closely, that could means the socket is corrupted or significant nw error occurs:
					 * in that case the system throw a Socket Timeout exception, otherwise, it will remains in starvation state.
					 */
					clientSocket.setSoTimeout(500);
					while (true) {					
						byte[] buffer = new byte[255];
						int bytesRead = clientSocket.getInputStream().read(buffer);
						if(bytesRead==-1) {
							System.out.println("Client disconnnected");
							break;
						}
						else for(byte b:buffer)
							System.out.println((int)b);
				
						// Insert received raw data into db
						String insertQuery = "INSERT INTO stove_log"
								+ "("
								+ "creation_date, "
								+ "datagram "
								+ ") "
								+ "VALUES (?,?);";


						try (Connection connection = DriverManager.getConnection(url, user, password);
								PreparedStatement preparedStatement = connection.prepareStatement(insertQuery)) {

							// Be aware: the generated time should have UTC value, check your host config
							preparedStatement.setTimestamp(1,new java.sql.Timestamp(new java.util.Date().getTime())); 
							preparedStatement.setBytes(2, buffer);

							// exec insert command
							int rowsInserted = preparedStatement.executeUpdate();
							if (rowsInserted > 0) {
								System.out.println("Insert done");
							}

						} catch (SQLException e) {
							System.err.println("An error occours while executing insert: " + e.getMessage());
						}

					}


				} catch (IOException e) {
					System.err.println("Errore nella comunicazione con il client: " + e.getMessage());
				}

		} catch (IOException e) {
			System.err.println("Errore nell'avviare il server: " + e.getMessage());
		}

	}
}