#include <iostream>
#include <thread>
#include <vector>
#include <semaphore>
#include <barrier>
#include <chrono>
#include <random>
#include <mutex>
#include <memory>
#include <string>
#include <sstream>
using namespace std;

mutex cout_mutex;

class BoardingGate {
public:
    BoardingGate(int gateId) : id(gateId), gateSemaphore(1) {}
    BoardingGate(const BoardingGate&) = delete;
    BoardingGate& operator=(const BoardingGate&) = delete;

    bool try_acquire() {
        return gateSemaphore.try_acquire();
    }

    void acquire() {
        gateSemaphore.acquire();
    }

    void release() {
        gateSemaphore.release();
    }

    int getId() const {
        return id;
    }

private:
    int id;
    counting_semaphore<1> gateSemaphore;
};

void boardPassenger(int groupId, int passengerId, int gateId) {
    {
        lock_guard lock(cout_mutex);
        cout << "Group " << groupId << " Passenger " << passengerId 
                  << " boarding at Gate " << gateId << endl;
    }
 
    this_thread::sleep_for(chrono::milliseconds(100 + (rand() % 200)));
    {
        lock_guard lock(cout_mutex);
        cout << "Group " << groupId << " Passenger " << passengerId 
                  << " finished boarding at Gate " << gateId << endl;
    }
}

void boardGroup(int groupId, int groupSize, shared_ptr<BoardingGate> gate) {
    barrier boardingBarrier(groupSize, [&gate, groupId](){
        lock_guard lock(cout_mutex);
        cout << "Group " << groupId << " completed boarding at Gate " << gate->getId() << ". Gate released." << endl;
        gate->release();
    });

    {
        lock_guard lock(cout_mutex);
        cout << "Group " << groupId << " started boarding at Gate " << gate->getId() << endl;
    }

    gate->acquire();

    vector<thread> passengers;
    for (int i = 1; i <= groupSize; ++i) {
        passengers.emplace_back([&, i]() {
            boardPassenger(groupId, i, gate->getId());
            boardingBarrier.arrive_and_wait();
        });
    }

    for (auto& t : passengers)
        t.join();
}

// Automated simulation
void automatedSimulation(vector<shared_ptr<BoardingGate>>& gates, const vector<int>& groupSizes) {
    srand((unsigned)time(nullptr));
    for (int groupId = 1; groupId <= (int)groupSizes.size(); ++groupId) {
        int size = groupSizes[groupId - 1];

        bool gateAssigned = false;
        while (!gateAssigned) {
            for (auto& gate : gates) {
                if (gate->try_acquire()) {
                    gate->release();
                    thread groupThread(boardGroup, groupId, size, gate);
                    groupThread.detach();
                    gateAssigned = true;
                    break;
                }
            }
            if (!gateAssigned) {
                this_thread::sleep_for(100ms);
            }
        }
    }

    int maxPassengers = 0;
    for (auto size : groupSizes)
        if (size > maxPassengers) maxPassengers = size;
    this_thread::sleep_for(chrono::milliseconds(500 * maxPassengers + 2000));
}

// Manual simulation
void manualSimulation(vector<shared_ptr<BoardingGate>>& gates) {
    srand((unsigned)time(nullptr));
    vector<int> groupSizes;
    int groupCount = 0;
    string input;

    cout << "Enter number of boarding groups: ";
    getline(cin, input);
    istringstream(input) >> groupCount;

    while (groupCount <= 0) {
        cout << "Invalid input. Enter a positive integer for group count: ";
        getline(cin, input);
        istringstream(input) >> groupCount;
    }

    for (int i = 0; i < groupCount; ++i) {
        int size = 0;
        cout << "Enter number of passengers for Group " << (i + 1) << ": ";
        getline(cin, input);
        istringstream(input) >> size;

        while (size <= 0) {
            cout << "Invalid input. Enter positive integer for passengers: ";
            getline(cin, input);
            istringstream(input) >> size;
        }
        groupSizes.push_back(size);
    }

    cout << "\nStarting manual boarding simulation...\n";

    for (int groupId = 1; groupId <= groupCount; ++groupId) {
        int size = groupSizes[groupId - 1];

        shared_ptr<BoardingGate> assignedGate = nullptr;
        while (!assignedGate) {
            for (auto& gate : gates) {
                if (gate->try_acquire()) {
                    assignedGate = gate;
                    gate->release();
                    break;
                }
            }
            if (!assignedGate) {
                this_thread::sleep_for(100ms);
            }
        }

        boardGroup(groupId, size, assignedGate);

        cout << "\nGroup " << groupId << " finished boarding. Press Enter to continue to next group or type 'exit' to quit: ";
        getline(cin, input);
        if (input == "exit" || input == "EXIT" || input == "Exit") {
            cout << "Exiting manual simulation.\n";
            return;
        }
    }

    cout << "All groups have boarded.\n";
}

int main() {
    vector<shared_ptr<BoardingGate>> gates;
    gates.emplace_back(make_shared<BoardingGate>(1));
    gates.emplace_back(make_shared<BoardingGate>(2));
    gates.emplace_back(make_shared<BoardingGate>(3));

    while (true) {
        cout << "\n=== Airline Boarding Simulation Menu ===\n"
                     "1. Manual Simulation\n"
                     "2. Automated Simulation\n"
                     "3. Exit\n"
                     "Enter your choice (1-3): ";

        int choice = 0;
        
        string input;
        
        getline(cin, input);
        
        istringstream(input) >> choice;

        if (choice == 1) {
            manualSimulation(gates);
        } else if (choice == 2) {
            vector<int> groupSizes = {5, 4, 6, 3, 7};
            automatedSimulation(gates, groupSizes);
        } else if (choice == 3) {
            cout << "Exiting program. Goodbye!\n";
            break;
        } else {
            cout << "Invalid choice. Enter 1, 2, or 3.\n";
        }
    }
    return 0;
}