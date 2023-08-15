// https://codingchallenges.substack.com/p/coding-challenge-22-dns-resolver
#include <iostream>
#include <iomanip> // For setw() and setfill()
#include <cstring>
#include <arpa/inet.h>  // For htons()
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>


// Define the DNS header structure
struct DNSHeader {
    uint16_t id;
    uint16_t flags;
    uint16_t numQuestions;
    uint16_t numAnswers;
    uint16_t numAuthority;
    uint16_t numAdditional;
};

// Define the DNS question structure
struct DNSQuestion {
    std::string name;
    uint16_t recordType;
    uint16_t queryClass;
};

struct DNSAnswer {
    std::string name;
    uint16_t type;
    uint16_t queryClass;
    uint32_t ttl;
    uint16_t dataLength;
    // Data content (e.g., IP address)
};

int main() {
    DNSHeader header;
    DNSQuestion question;
    DNSAnswer answer[2]; // Assuming 2 answers, adjust as needed

    // Fill in header fields
    header.id = htons(22); // Use random ID
    header.flags = htons(0x0100); // Recursion desired bit set
    header.numQuestions = htons(1);
    header.numAnswers = htons(0);
    header.numAuthority = htons(0);
    header.numAdditional = htons(0);

    // Fill in question fields
    question.name = "dns.google.com"; // Change to your desired hostname
    question.recordType = htons(1); // Query type A (IPv4 address)
    question.queryClass = htons(1); // Query class IN (Internet)

    // Convert structs to byte strings
    std::string dnsMessage;
    dnsMessage.append(reinterpret_cast<const char*>(&header), sizeof(header));

    // Encode the question name
    std::string encodedName;
    const char* namePart = question.name.c_str();
    while (*namePart) {
        char length = 0;
        std::string label;
        while (*namePart && *namePart != '.') {
            label += *namePart;
            ++namePart;
            ++length;
        }
        if (length > 0) {
            encodedName += length;
            encodedName += label;
        }
        if (*namePart == '.') {
            ++namePart;
        }
    }
    encodedName += '\0'; // Terminating null byte
    dnsMessage += encodedName;

    // Add query type and query class
    dnsMessage.append(reinterpret_cast<const char*>(&question.recordType), sizeof(question.recordType));
    dnsMessage.append(reinterpret_cast<const char*>(&question.queryClass), sizeof(question.queryClass));

    // Print the final DNS message as hex
    for (char c : dnsMessage) {
        std::cout << std::hex << std::setw(2) << std::setfill('0') << static_cast<int>(static_cast<uint8_t>(c));
    }
    std::cout << std::endl;

    // Create a UDP socket
    int sockfd = socket(AF_INET, SOCK_DGRAM, 0);
    if (sockfd == -1) {
        perror("socket");
        return 1;
    }

    // Server address information
    struct sockaddr_in serverAddr;
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_port = htons(53); // DNS service port
    serverAddr.sin_addr.s_addr = inet_addr("8.8.8.8"); // Google's DNS server

    // Send the DNS message
    ssize_t sentBytes = sendto(sockfd, dnsMessage.data(), dnsMessage.size(), 0,
                               reinterpret_cast<struct sockaddr*>(&serverAddr),
                               sizeof(serverAddr));

    if (sentBytes == -1) {
        perror("sendto");
        close(sockfd);
        return 1;
    }

    // Receive and process response
    char responseBuffer[1024]; // Adjust buffer size if needed
    struct sockaddr_in clientAddr;
    socklen_t clientAddrLen = sizeof(clientAddr);
    ssize_t receivedBytes = recvfrom(sockfd, responseBuffer, sizeof(responseBuffer), 0,
                                     reinterpret_cast<struct sockaddr*>(&clientAddr),
                                     &clientAddrLen);
    if (receivedBytes == -1) {
        perror("recvfrom");
        close(sockfd);
        return 1;
    }

    // Process the response data and print it out
    for (ssize_t i = 0; i < receivedBytes; ++i) {
        std::cout << std::hex << std::setw(2) << std::setfill('0')
                  << static_cast<int>(static_cast<uint8_t>(responseBuffer[i]));
    }
    std::cout << std::endl;

    // Close the socket
    close(sockfd);
    return 0;
}
