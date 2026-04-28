// Nim Project (COMP 3110)
// NIM 7
// Sofi Sarmiento
// Carter Owens
// Josiah Duvalian
// Ellieze Hood

// Game logic notes for us te remember!!!!!
// Server picks 3 to 9 piles, each with 1 to 20 rocks. The board is sent as one string like "3030405" 
//      where first character is pile count, then every two characters are a zero padded rock count for each pile.
// Always the challenger (client) goes first. ALWAYS. The host sets the board, the client makes the first move.
// To move you pick one pile, take at least 1 rock, at most however many are left in it. 
//      One pile per turn, and the goal is to take the last rock.
// No "you lost" message, because both sides track the board themselves. When you see the last rock get taken, you know. 
// Chats happen before your move. On your turn, before you submit a move, you can send as many chat messages as you want. 
//      They start with 'C' on the wire but the 'C' gets stripped before displaying. You do not switch turns.
// Forfeit ends it instantly so sending "F" means you lose, receiving "F" means you win. 
// Invalid move is an automatic loss for the sender. Wrong pile, too many rocks, bad format, anything, the receiver wins by default immediately. 
//      Same goes for taking more than 30 seconds to move. This protects each team's code from bugs in other teams' code lol.
// After every game, both sides go back to the menu which is gonna be gui's job. 
//      The protocol says once a game concludes, both players get offered host or challenge again. 

#include <iostream>
#include <cstring>
#include <cstdlib>
#include <ctime>
#include <WinSock2.h>
#include "Nim.h"
#include "nimGame.h"


void initGame(GameState& gs, bool iAmClient) {
    srand((unsigned int)time(nullptr));

    // Server gets how many piles and how many rocks
    // Client just needs the turn flags set 
    if (!iAmClient) {
        gs.numPiles = (rand() % 7) + 3; // random 3 to 9
        for (int i = 0; i < gs.numPiles; i++) {
            gs.piles[i] = (rand() % 20) + 1; // random 1 to 20
        }
    }

    gs.iAmClient = iAmClient;
    gs.myTurn = iAmClient;   // challenger always goes first
    gs.status = STATUS_ACTIVE;
}


// GUI might call validateOutgoingMove when the local player selects a move, BEFORE sending it.
// If return is MOVE_OK, tell networking to send the move
// If return is MOVE_BAD_, show an error, ask the user to try again

MoveResult validateOutgoingMove(const GameState& gs, int pile, int count) {
    if (pile < 1 || pile > gs.numPiles) {
        return MOVE_BAD_PILE;
    }
    if (gs.piles[pile - 1] == 0) {
        return MOVE_BAD_PILE;
    }
    if (count < 1 || count > gs.piles[pile - 1]) {
        return MOVE_BAD_COUNT;
    }
    return MOVE_OK;
}

// Networking call validateIncomingMove every time a datagram arrives from the opponent during gameplay.
// datagram is basically the raw C-string you received
// if MOVE_OK, call applyMove(gs, outPile, outCount), then checkWin()
// if MOVE_CHAT, strip the leading 'C', display the rest. 
// if MOVE_FORFEIT, call endGame(gs, STATUS_FORFEIT_WIN)
// if MOVE_BAD_*, call endGame(gs, STATUS_WIN_DEFAULT). Opponent broke rules :(

MoveResult validateIncomingMove(const GameState& gs, const char* datagram, int& outPile, int& outCount) {
    if (datagram == nullptr || strlen(datagram) == 0) {
        return MOVE_BAD_FORMAT;
    }

    char first = datagram[0];

    if (first == 'F') {
        return MOVE_FORFEIT;
    }
    if (first == 'C') {
        return MOVE_CHAT;
    } 

    // must be a digit 1-9 for a move
    if (first < '1' || first > '9') {
        return MOVE_BAD_FORMAT;
    }

    if (strlen(datagram) != 3) {
        return MOVE_BAD_FORMAT;
    }
    
    if (!isdigit(datagram[1]) || !isdigit(datagram[2])) {
        return MOVE_BAD_FORMAT;
    }

    outPile  = first - '0';                              
    outCount = (datagram[1] - '0') * 10 + (datagram[2] - '0');

    if (outPile < 1 || outPile > gs.numPiles) {
        return MOVE_BAD_PILE;
    }
    if (gs.piles[outPile - 1] == 0) {
        return MOVE_BAD_PILE;
    }
    if (outCount < 1 || outCount > gs.piles[outPile - 1]) {
        return MOVE_BAD_COUNT;
    }

    return MOVE_OK;
}

// Call the applyMOve after validateIncomingMove() returns MOVE_OK
// Removes rocks from the pile and flips whose turn it is.
// After calling this, always call checkWin() to see if the game is over.

void applyMove(GameState& gs, int pile, int count) {
    gs.piles[pile - 1] -= count;   
    gs.myTurn = !gs.myTurn;      
}

bool checkWin(const GameState& gs) {
    for (int i = 0; i < gs.numPiles; i++) {
        if (gs.piles[i] > 0) {
            return false;
        }
    }
    return true;
}

// end game, show message of win/lose, then show menu again
void endGame(GameState& gs, GameStatus reason) {
    gs.status = reason;
}

void resetGame(GameState& gs) {
    for (int i = 0; i < 9; i++) {
        gs.piles[i] = 0;
    }
    gs.numPiles = 0;
    gs.myTurn = false;
    gs.status = STATUS_WAITING;
}

// if true, board is valid, call initGame() and start playing
// if false, board is invalid, call endGame

bool parseBoardDatagram(const char* datagram, int piles[], int& numPiles) {
    if (datagram == nullptr || strlen(datagram) < 3) {
        return false;
    }

    if (!isdigit(datagram[0])) {
        return false;
    }

    numPiles = datagram[0] - '0';
    if (numPiles < 3 || numPiles > 9) {
        return false;
    }

    if ((int)strlen(datagram) != 1 + numPiles * 2) {
        return false;
    }

    for (int i = 0; i < numPiles; i++) {
        char hi = datagram[1 + i * 2];
        char lo = datagram[2 + i * 2];
        if (!isdigit(hi) || !isdigit(lo)) {
            return false;
        }

        int rocks = (hi - '0') * 10 + (lo - '0');
        if (rocks < 1 || rocks > 20) {
            return false;
        }
        piles[i] = rocks;
    }

    return true;
}

void buildBoardDatagram(const GameState& gs, char* outBuf) {
    int pos = 0;
    outBuf[pos++] = '0' + gs.numPiles; // first char = pile count
    for (int i = 0; i < gs.numPiles; i++) {
        outBuf[pos++] = '0' + (gs.piles[i] / 10); // tens digit
        outBuf[pos++] = '0' + (gs.piles[i] % 10); // ones digit
    }
    outBuf[pos] = '\0';
}


// For example if pile 2, remove 5 rocks is something like "205\0"
void buildMoveDatagram(int pile, int count, char* outBuf) {
    outBuf[0] = '0' + pile;         // pile digit
    outBuf[1] = '0' + (count / 10); // tens digit of count
    outBuf[2] = '0' + (count % 10); // ones digit of count
    outBuf[3] = '\0';
}

// Example of thw whole thing in a main function or something, just to show how everything fit together
void exampleTurnLoop(GameState& gs) {
    while (gs.status == STATUS_ACTIVE) {
        if (gs.myTurn) {
            // Show the board
            // Optionally let user type a chat message ("C" and message string) (send datagram)
            // Optionally let user forfeit (exit by sending "F", call endGame, break)

            // Ask user for pile number and rock count
            int pile  = 0; // get from user input
            int count = 0; // get from user input

            MoveResult r = validateOutgoingMove(gs, pile, count);
            if (r == MOVE_BAD_PILE)  { /* show "invalid pile" error */ continue; }
            if (r == MOVE_BAD_COUNT) { /* show "invalid count" error */ continue; }

            char moveBuf[4];
            buildMoveDatagram(pile, count, moveBuf);
            // send moveBuf as UDP datagram to opponent
            applyMove(gs, pile, count);
            // update the board display
            if (checkWin(gs)) {
                endGame(gs, STATUS_WIN);
                // show winning message
                break;
            }
        } else {

            // gui shows a waiting message
            char recvBuf[256] = {0};

            int pile = 0, count = 0;
            MoveResult r = validateIncomingMove(gs, recvBuf, pile, count);

            if (r == MOVE_CHAT) {
                continue; // keep waiting, don't switch turns, just chat
            }

            if (r == MOVE_FORFEIT) {
                endGame(gs, STATUS_FORFEIT_WIN);
                break;
            }

            if (r != MOVE_OK) {
                endGame(gs, STATUS_WIN_DEFAULT);
                // gui show something about the opponent sending an invalid move ans it's a win by default
                break;
            }

            applyMove(gs, pile, count);
            // gui update the board display

            if (checkWin(gs)) {
                endGame(gs, STATUS_LOSE);
                // gui show losing message
                break;
            }
        }
    }
}