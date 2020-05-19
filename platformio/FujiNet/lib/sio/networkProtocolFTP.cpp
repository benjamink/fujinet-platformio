#include "networkProtocolFTP.h"
#include "../../include/debug.h"

bool networkProtocolFTP::ftpExpect(string resultCode)
{
    char buf[512];
    string sbuf;

    if (!control.connected())
        return false;

    control.readBytesUntil('\n', buf, sizeof(buf));
    sbuf = string(buf);
    controlResponse = sbuf.substr(4);

    Debug_printf("Got response: %s\n", sbuf.c_str());
    Debug_printf("Returning response: %s\n", controlResponse.c_str());
    return (sbuf.find_first_of(resultCode) == 0 ? true : false);
}

unsigned short networkProtocolFTP::parsePort(string response)
{
    size_t pos_start = 0;
    size_t pos_end = 0;
    unsigned short port;

    pos_start = response.find_first_of("|");
    pos_start += 2;

    Debug_printf("pos_start %d\n", pos_start);

    pos_end = response.find_last_of("|");
    Debug_printf("pos_end %d\n", pos_end);
    pos_start++;
    pos_end--;
    port = (atoi(response.substr(pos_start, pos_end).c_str()));
    Debug_printf("port string %s\r\n", response.substr(pos_start, pos_end));
    Debug_printf("Parsed port is: %d\n", port);
    return port;
}

networkProtocolFTP::networkProtocolFTP()
{
}

networkProtocolFTP::~networkProtocolFTP()
{
}

bool networkProtocolFTP::open(EdUrlParser *urlParser, cmdFrame_t *cmdFrame)
{
    if (urlParser->port.empty())
        urlParser->port = "21";

    hostName = urlParser->hostName;

    if (!control.connect(urlParser->hostName.c_str(), atoi(urlParser->port.c_str())))
        return false; // Error

    if (!ftpExpect("220"))
        return false; // error

    control.write("USER anonymous\r\n");

    if (!ftpExpect("331"))
        return false;

    control.write("PASS fujinet@fujinet.online\r\n");

    if (!ftpExpect("230"))
        return false;

    control.write("TYPE I\r\n");

    if (!ftpExpect("200"))
        return false;

    aux1 = cmdFrame->aux1;

    switch (cmdFrame->aux1)
    {
    case 4:
        control.write("SIZE ");
        control.write(urlParser->path.c_str());
        control.write("\r\n");

        if (!ftpExpect("213"))
            return false;

        dataSize = atol(controlResponse.c_str());

        control.write("EPSV\r\n");

        if (!ftpExpect("229"))
            return false;

        dataPort = parsePort(controlResponse);

        control.write("RETR ");
        control.write(urlParser->path.c_str());
        control.write("\r\n");
        break;
    case 6:
        control.write("EPSV\r\n");

        if (!ftpExpect("229"))
            return false;

        dataPort = parsePort(controlResponse);

        control.write("NLST ");
        control.write(urlParser->path.c_str());
        control.write("\r\n");

        break;
    }

    if (!data.connect(hostName.c_str(), dataPort))
        return false;

    delay(500);

    return true;
}

bool networkProtocolFTP::close()
{
    if (data.connected())
        data.stop();

    if (control.connected())
    {
        control.write("QUIT\r\n");
    }

    ftpExpect("221");

    control.stop();
    return true;
}

bool networkProtocolFTP::read(byte *rx_buf, unsigned short len)
{
    if (data.readBytes(rx_buf, len) != len)
        return true;
    else
        dataSize -= len;

    if (aux1==6)
    {
        for (int i=0;i<len;i++)
            {
                if (rx_buf[i]==0x0D)
                    rx_buf[i]=0x20;
                else if (rx_buf[i]==0x0A)
                    rx_buf[i]=0x9B;
            }
    }
    return false;
}

bool networkProtocolFTP::write(byte *tx_buf, unsigned short len)
{
    return false;
}

bool networkProtocolFTP::status(byte *status_buf)
{
    status_buf[0] = data.available() & 0xFF;
    status_buf[1] = data.available() >> 8;
    status_buf[2] = 1;
    status_buf[3] = 1;
    return false;
}

bool networkProtocolFTP::special(byte *sp_buf, unsigned short len, cmdFrame_t *cmdFrame)
{
    return false;
}

bool networkProtocolFTP::special_supported_00_command(unsigned char comnd)
{
    return false;
}