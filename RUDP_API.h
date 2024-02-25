#pragma once

/*
 * Creating a RUDP socket and creating a handshake between two peers.
 */
// rudp_socket();

/*
 * Sending data to the peer. The function should wait for an
 * acknowledgment packet, and if it didn’t receive any, retransmits the data.
 */
// rudp_send();

/*
 * Receive data from a peer.
 */
// rudp_rcv();

/*
 * Closes a connection between peers.
 */
// rudp_close();