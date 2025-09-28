## Description

This is a simple chat made in C. You have various commands availables,
such as:

- /help : display the help menu
- /user : display your username
- /dir : get the current operating directory
- /ip : get the server's public IP address
- /reboot : restart the server (entire system)
- /quit : exit the chat

## How to use

To send a message, you just need to enter your username when connecting, and then enter the string you want to send.
You can receive the message + the user who sent it.
To compile the files, open a shell in the repository and enter `make all`.
Then, type `./server` for the server or `./client` for the client.

## Requirements

- gcc compiler (`sudo apt install gcc`)
