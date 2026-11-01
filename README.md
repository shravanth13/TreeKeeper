# TreeKeeper

**A Data-Driven Afforestation and Maintenance Scheduling System**
Team **PROTOCOL 0** · CSLR31 Data Structures Laboratory · NIT Tiruchirappalli
SDG 15 – Life on Land

| Member | Reg. No. |
|---|---|
| Shaurya Bharat Makwana | 106125109 |
| Shravanth S Sennimalai | 106125111 |
| Shreya Ashok Mhetre    | 106125113 |

TreeKeeper keeps track of planted saplings and tells volunteers which trees need water first. It also finds the shortest walking route between zones of the campus. A timer runs in the background and keeps reducing the water level of every tree. All data is saved to files when the program exits and loaded back the next time.

## How to run

```sh
make
./a.out
```

The first time, the program starts with an empty campus. Add zones (option 8), paths (option 9), volunteers (option 12) and then plant trees (option 2). Everything is saved in the `data/` folder and loaded back the next time.

## SETUP START (LINUX)
```sh
ls -la
rm -rf cc
sudo apt update
sudo apt install build-essential
make
./a.out
```

## WSL SETUP START
```sh
bash
ls -la
rm -rf cc
sudo apt update
sudo apt install build-essential
make
./a.out
```

## Folder structure

```
Makefile          build file (a.out is created here)
README.md
include/          header files (.h)
Source/           source files (.c)
object/           object files (.o), created by make
Tests/            test program and sample input
Document/         user manual
Video/            project video
data/             saved data (created when the program exits)
```

| File | What it does |
|---|---|
| `tree.c` | Tree struct, species list (8 built in, more can be added), status of a tree, hash map (separate chaining) to find a tree by ID |
| `heap.c` | Min heap to find the most urgent tree, background timer thread (pthreads) that dries the trees |
| `graph.c` | Campus graph (adjacency list), Dijkstra, route planning |
| `volunteer.c` | Volunteer list and leaderboard |
| `fileio.c` | Saving and loading CSV files |
| `ui.c` | Dashboard with colours, input functions (declared in `treekeeper.h`) |
| `main.c` | Menu |

## Data structures used

**Min heap:** each tree's priority is the number of hours left before it runs out of water (`water / dryRate`). The tree with the smallest value is at the root. Every tree stores its position in the heap (`heapIndex`), so when a tree is watered we can move it to its correct place in O(log n) without searching the whole heap. When the timer reduces all water levels, the heap is rebuilt with `buildHeap` in O(n).

**Hash map:** the key is the tree ID, and collisions are handled with linked lists (separate chaining). When a volunteer scans the QR code, the tree is found in O(1) on average. When the number of trees goes above 75% of the number of buckets, the table doubles in size and every tree is rehashed.

**Graph:** zones are vertices and paths are undirected weighted edges (distance in metres), stored as an adjacency list. Dijkstra's algorithm (array version, O(V²)) finds the shortest path, and the path is printed using the parent array. To assign several trees, the program repeatedly walks to the nearest unvisited zone (greedy approach).

**Dynamic arrays:** the heap array, the zone array and the volunteer list double their size with `realloc` when they are full.

| Operation | Time |
|---|---|
| Search tree by ID | O(1) average |
| Plant / water / remove a tree | O(log n) |
| Get most urgent tree | O(1) peek, O(log n) extract |
| Timer update | O(n) |
| Shortest path | O(V²) |

## Threads

`heap.c` also starts a thread with `pthread_create`. Every second it reduces the water level of all trees by `dryRate × speed`. Both the menu and the timer thread use the trees and the heap, so every access is protected with a `pthread_mutex_t`. On exit, `running` is set to 0 and `pthread_join` waits for the thread to finish.

## Saved files

| File | Format |
|---|---|
| `settings.csv` | simHours,nextTreeId,nextVolunteerId,speed |
| `species.csv` | name,dryRate |
| `zones.csv` | one zone name per line |
| `paths.csv` | zoneA,zoneB,distance |
| `trees.csv` | id,species,zone,water,diseased,assignedTo |
| `volunteers.csv` | id,name,zone,tasksDone |

Lines that cannot be read (wrong format, zone that does not exist, water outside 0–100) are skipped, and a warning is printed.

## Testing

`make test` runs `Tests/test.c` (40 test cases). It covers adding species, the timer update (including a wilted assigned tree going back to the queue), inserting, searching and deleting in the hash map (including rehashing and duplicate IDs), heap insert/update/remove/extract order, Dijkstra on a small graph (including an unreachable zone and a removed path), and saving data and loading it back. The sample campus (10 zones, 16 trees, 3 volunteers) used for this is inside `test.c`, so it is only loaded by `make test`.

`./a.out < Tests/test_input.txt` runs a full session from the menu. It includes wrong inputs such as an unknown species, an unknown zone, a missing tree ID and text instead of a number.

We checked memory with `leaks --atExit` on macOS (0 leaks). On Linux, use `make debug` and then `valgrind --leak-check=full ./a.out`.

## Future scope

- IoT soil moisture sensors instead of the simulated timer
- Disease detection from leaf photos
- Mobile app for scanning QR codes

## References

- Cormen et al., *Introduction to Algorithms* – heaps, hash tables, Dijkstra
- GeeksforGeeks – Dijkstra's algorithm (adjacency list), separate chaining
- POSIX threads man pages (`pthread_create`, `pthread_mutex_lock`)
