#include "state.hpp"

void SessionWorker() {
    for (;;) {
        // The slot is shared across translation units. Reacquire after WAIT.
        if (auto* state = SharedSession::Get()) ++state->worker_steps;
        WAIT(100);
    }
}
