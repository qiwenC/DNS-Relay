# DNS-Relay

An implementation of DNS relay on Linux

## Requirement

## Environment configuration

### nslookup

The `nslookup` stands for name server lookup, which is a command that helps to look up the ip address corresponding to the domain name

The syntax is `nslookup THEDOMAINNAME`

for example `nslookup www.bupt.edu.cn`

This command helps to check the result of the DNS relay

### DNS server address

To use the local address as the DNS relay, the DNS address need to be changed to the local address.

Under the super model (you can use `su` to be in the root model), enter `ifconfig`, you will find the local ip address

Then enter `vi \etc\resolv.conf`, it will show the text about the DNS address. Changing the address to the local ip address.

### compile

To compile the program, enter `gcc -o dnsrelay dnsrelay.c`

Then, under the super model, enter `./dnsrelay` to start the program

### troubleshoot

- If the dns address cannot save or bind fail, check whether the system is under the super model
- When using `vi` to edit the file, using `x` to delete character, `i` to add new information and `:wq` to quit and save 
- Using control+C to end the program
