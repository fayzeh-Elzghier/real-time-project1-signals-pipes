# Real-Time and Embedded Systems Project 1  
## Furniture Moving Competition Using Processes, Pipes, Signals, FIFOs, and OpenMP

## 1. Project Overview

This project simulates a furniture moving competition between two teams.

Each team has a number of members defined by the user. The first member of each team acts as the source, and the last member acts as the sink/house. Furniture pieces must be moved from the source to the sink through the team members.

The sink only accepts furniture pieces in the correct order. If a wrong piece reaches the sink, it is returned backward through the same team path. The team that successfully moves all furniture pieces in order first wins the round. The competition continues until one team reaches the configured maximum number of wins.

---

## 2. Main Features Implemented

The project currently supports:

- Two competing teams.
- User-defined configuration through `config.txt`.
- Multiple processes using `fork()`.
- Pipes for communication between team members.
- Forward movement of furniture pieces.
- Backward return messages from the sink.
- Random furniture piece selection.
- Blocking rule: a returned wrong piece cannot be selected again immediately.
- Random delay for each movement.
- Increasing delay due to tiredness.
- Parallel team competition.
- Start signal barrier using `SIGUSR1`.
- Stopping the losing team using `SIGTERM`.
- Multi-round competition until `MAX_WINS`.
- FIFO logging for important events.
- OpenMP usage for safe parallel initialization.

---

## 3. Project Structure

```text
project1/
├── Makefile
├── README.md
├── config.txt
└── src/
    ├── config.c
    ├── config.h
    ├── ipc.c
    ├── ipc.h
    ├── main.c
    ├── team.c
    └── team.h
4. Configuration File

The project reads parameters from config.txt.

Example:

TEAM_MEMBERS=3
FURNITURE_PIECES=5
MAX_WINS=2
MIN_DELAY=0
MAX_DELAY=1
Meaning of each value
Parameter	Meaning
TEAM_MEMBERS	Number of members in each team
FURNITURE_PIECES	Number of furniture pieces that must be moved
MAX_WINS	Number of round wins needed to win the competition
MIN_DELAY	Minimum random delay in seconds
MAX_DELAY	Maximum random delay in seconds
5. How to Compile and Run

Compile the project:

make clean
make

Run the project:

./project1 config.txt
6. IPC Design

This project uses three IPC mechanisms:

Pipes
Signals
FIFOs
6.1 Pipes

Pipes are used for direct communication between team members.

Each team has two directions of communication:

Forward pipes

Used to move furniture pieces from the source to the sink.

Source Member → Middle Members → Sink Member

The data sent forward is:

typedef struct {
    int piece_id;
    int team_id;
    int direction;
} FurniturePiece;
Backward pipes

Used to return the result from the sink back to the source.

Sink Member → Middle Members → Source Member

The data sent backward is:

typedef struct {
    int status;
    FurniturePiece piece;
} ReturnMessage;

The return message tells the source whether the piece was accepted or rejected.

6.2 Signals

Signals are used for round control.

SIGUSR1

Used as a start barrier.

Both teams are created first, then each team waits for the start signal. The parent process sends SIGUSR1 to both teams so they start the round at almost the same time.

Parent → SIGUSR1 → Team 1
Parent → SIGUSR1 → Team 2

This makes the competition fairer.

SIGTERM

Used to stop the losing team.

When one team finishes the round first, the parent process sends SIGTERM to the other team’s process group.

Parent → SIGTERM → losing team process group

This stops the losing team immediately after the round winner is known.

6.3 FIFO

A FIFO is used for logging important events.

The FIFO path is:

/tmp/project1_logs_fifo

The simulation processes write log messages to the FIFO, and a logger process reads and prints them.

Example log messages:

[Team 1] Source selected piece 3.
[Team 2] Sink accepted piece 1.
[Team 1] Sink rejected piece 4, expected 2.
[Round 2] Team 1 won the round.

The FIFO is not used to move furniture pieces. It is used only for logging and monitoring important events.

7. Furniture Piece Selection Logic

The source member randomly selects a furniture piece from the available pieces.

At the beginning, all pieces are available:

available[i] = 1

This means the piece can still be selected.

When a piece is accepted by the sink, it becomes unavailable:

available[i] = 0

This prevents accepted pieces from being selected again.

8. Blocked Piece Rule

If a wrong piece reaches the sink, it is returned backward.

The source stores this piece as:

blocked_piece

The same wrong piece cannot be selected again immediately.

Example:

Expected piece: 2
Source sends piece 5
Sink rejects piece 5
Source blocks piece 5 for the next selection

The blocked piece becomes selectable again only after a different successful move.

9. Sink Acceptance Logic

The sink accepts pieces only in order.

Example for 5 furniture pieces:

Expected order: 1 → 2 → 3 → 4 → 5

If the sink expects piece 2 and receives piece 4:

WRONG

If the sink expects piece 2 and receives piece 2:

ACCEPTED

After accepting a piece, the sink updates the expected piece:

expected_piece++;
10. Delay and Tiredness

Each team member delays before passing a piece or a return message.

The base delay is random between:

MIN_DELAY and MAX_DELAY

A tiredness factor is also added. The more a member moves pieces/messages, the more delay is added.

Example:

int tiredness_delay = moved_count / 20;

This simulates team members becoming tired over time.

11. OpenMP Usage

OpenMP is used for safe parallel initialization of the available pieces array.

#pragma omp parallel for
for (int i = 0; i < num_pieces; i++) {
    available[i] = 1;
}

This loop is safe to parallelize because each thread writes to a different array index.

OpenMP is not used inside pipe communication logic because that part depends on process synchronization and IPC.

12. Competition Flow

The overall flow is:

1. Read configuration.
2. Create FIFO logger.
3. Start competition.
4. For each round:
   a. Fork Team 1 round process.
   b. Fork Team 2 round process.
   c. Both teams wait for SIGUSR1.
   d. Parent sends SIGUSR1 to both teams.
   e. Both teams start moving furniture pieces.
   f. First team to finish wins the round.
   g. Parent stops the losing team using SIGTERM.
   h. Winner score is increased.
5. Repeat until one team reaches MAX_WINS.
6. Print final winner and score.
7. Stop logger and remove FIFO.
13. Sample Output

Example final output:

Round 1 winner: Team 1

Current Score:
Team 1: 1 wins
Team 2: 0 wins

Round 2 winner: Team 2

Current Score:
Team 1: 1 wins
Team 2: 1 wins

Round 3 winner: Team 2

Competition Winner: Team 2
Final Score:
Team 1: 1 wins
Team 2: 2 wins
14. Notes
The console output may appear interleaved because multiple processes are running at the same time.
Interleaved output is expected in a multiprocessing simulation.
FIFO logs are used to highlight important events.
The losing team is stopped after each round using signals.
The current implementation focuses on the console simulation and IPC correctness.