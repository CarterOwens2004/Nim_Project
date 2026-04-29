# Nim (Client/Server) — COMP 3110

## Team Members and Roles
* Sofia Sarmiento Ruiz: Game logic (win/loss detection, data structures)
* Carter Owens: GUI (menus, board, chat display)
* Josiah Duvalian: Networking - Server side (socket setup, hosting)
* Ellieze Hood: Networking - Client side (broadcast discovery, client negotiation, challenging)


## How to Play
Thanks for playing our game!!
When the program starts it will ask for your name (up to 80 characters). This is what other players on the network will see when you challenge them.
You will then be asked whether you want to **host a game** or **challenge someone**.

### Hosting
The program listens on UDP port 29333 for incoming players. When someone finds you and sends a challenge, you will be shown their name and asked to accept or decline. If you accept, you configure the board (3 to 9 piles, 1 to 20 rocks each) and the game begins. The challenger always makes the first move.

### Challenging
The program broadcasts a discovery message across the local subnet and collects responses for 2 seconds. Any available hosts will appear as a numbered list. Pick one, and the program sends them a challenge. You have 10 seconds to get a response. If they accept, you move first.

### Gameplay
The board shows each pile and how many rocks are in it. On your turn you pick a pile and how many rocks to remove (at least 1, at most however many are in the pile). You can also send a chat message or forfeit before making your move. The player who takes the last rock wins.

## Project File Structure (.zip)
```
Nim_Project/
├── Nim/
│   ├── Nim.h                # Constants and protocol strings
│   ├── nimGame.h            # GameState struct, enums, function declarations
│   ├── nim.cpp              # Game logic implementations
│   ├── nimNetwork.cpp       # Server and client networking
│   ├── Utilities.cpp        # Broadcast helpers, wait(), GetBroadcastAddress()
```
Nim.h and Utilities.cpp were renamed and adjusted from StudyBuddy
