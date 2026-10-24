// Native gameplay adaptation of root-cause/v-pizzaboy (MIT).
#include "locations.hpp"
#include "port.hpp"
using namespace Port;
static Ped customers[4]{};
static Blip pins[4]{};
static Vehicle bike = 0;
static Entity box = 0;
static bool running = false;
static int shop = 0, boxes = 5;
static unsigned delivered = 0, expected = 0;
static uint32_t endAt = 0, thrownAt = 0;
static Blip storePin = 0;
Vector3 Shop(int i) {
    auto p = Data::pizzaStores[i];
    return V(p.x, p.y, p.z);
}
void Stop() { // Return spawned actors/vehicles to clean state; no ambient actor is touched.
    ClearOwned();
    for (auto &c : customers)
        c = 0;
    for (auto &b : pins)
        b = 0;
    bike = 0;
    box = 0;
    storePin = 0;
    running = false;
    delivered = expected = 0;
}
bool Start() {
    Vector3 player = ENTITY::GET_ENTITY_COORDS(PedSelf(), true);
    int closest = -1;
    float best = 225;
    for (int i = 0; i < 4; ++i) {
        float d = Dist2(player, Shop(i));
        if (d < best) {
            best = d;
            closest = i;
        }
    }
    if (closest < 0) {
        Notice("Approach a pizza shop; use Locate store");
        return false;
    }
    if (Car()) {
        Notice("Exit your vehicle before starting the job");
        return false;
    }
    Stop();
    shop = closest;
    bike = VehicleAt("faggio", player, ENTITY::GET_ENTITY_HEADING(PedSelf()));
    if (!bike)
        return false;
    Hash pizza = H("prop_pizza_box_01");
    if (!Model(pizza)) {
        Stop();
        return false;
    }
    Release(pizza);
    for (int i = 0; i < 4; ++i) {
        Vector3 safe{};
        bool ok = false;
        for (int attempt = 0; attempt < 8 && !ok; ++attempt) {
            Vector3 p = Add(player, V(MISC::GET_RANDOM_FLOAT_IN_RANGE(-250.f, 250.f),
                                      MISC::GET_RANDOM_FLOAT_IN_RANGE(-250.f, 250.f), 0));
            ok = PATH::GET_SAFE_COORD_FOR_PED(p, true, &safe, 13) && Dist2(player, safe) > 2500;
        }
        if (!ok) {
            continue;
        }
        customers[i] = Own(PED::CREATE_RANDOM_PED(safe));
        if (customers[i]) {
            PED::SET_BLOCKING_OF_NON_TEMPORARY_EVENTS(customers[i], true);
            TASK::TASK_STAND_STILL(customers[i], 300000);
            pins[i] = Pin(safe, 1, 2);
        }
    }
    unsigned count = 0;
    for (Ped c : customers)
        if (c)
            ++count;
    if (!count) {
        Stop();
        Notice("No safe customer locations available");
        return false;
    }
    expected = count;
    PED::SET_PED_INTO_VEHICLE(PedSelf(), bike, -1);
    storePin = Pin(Shop(shop), 267, 5);
    boxes = 5;
    endAt = Now() + 300000;
    running = true;
    Report("delivery_started");
    return true;
}
void Throw(bool left) {
    if (!running || box || Car() != bike || boxes < 1) {
        Notice(!running    ? "Start a delivery shift first"
               : box       ? "Wait for the previous box to land"
               : boxes < 1 ? "No boxes left: return to the shop to restock"
                           : "Return to your delivery Faggio first");
        return;
    }
    Vector3 p = ENTITY::GET_OFFSET_FROM_ENTITY_IN_WORLD_COORDS(bike, V(left ? -1.f : 1.f, .5f, .5f));
    box = Prop("prop_pizza_box_01", p, {}, false);
    if (box) {
        --boxes;
        thrownAt = Now();
        Vector3 base = ENTITY::GET_ENTITY_COORDS(bike, true),
                point =
                    ENTITY::GET_OFFSET_FROM_ENTITY_IN_WORLD_COORDS(bike, V(left ? -10.f : 10.f, 0, 2.f));
        ENTITY::SET_ENTITY_VELOCITY(box, V(point.x - base.x, point.y - base.y, point.z - base.z));
    }
}
void ScriptMain() {
    Stop();
    App app(5, "Pizzaboy NX");
    int32_t store = 0;
    const char *names[] = {"Downtown Vinewood", "Vinewood East", "Mission Row", "Vespucci"};
    Report("ready");
    for (;;) {
        if (!app.Frame()) {
            if (running && ENTITY::IS_ENTITY_DEAD(PedSelf(), false))
                Stop();
            WAIT(100);
            continue;
        }
        unsigned remainingCustomers = 0;
        for (Ped c : customers)
            if (Exists(c))
                ++remainingCustomers;
        TextBuffer<96> job;
        if (running)
            job.UInt(unsigned(boxes))
                .Add(" boxes | ")
                .UInt(remainingCustomers)
                .Add(" customers | ")
                .UInt(Due(Now(), endAt) ? 0 : (endAt - Now()) / 1000)
                .Add("s");
        app.Status(running ? "ON SHIFT" : "OFF SHIFT",
                   running ? job.CStr() : "Locate a shop, approach on foot, start a shift");
        if (app.Open()) {
            app.menu.Choice(1, "Pizza shop", store, names, 4);
            app.menu.Action(2, "Locate store");
            app.menu.Action(3, "Start delivery shift",
                            "On foot within 15 metres of a pizza shop. Five minutes, five boxes, up to "
                            "four customers.",
                            !running);
            app.menu.Action(4, "Throw pizza left", "While seated on the job Faggio.", running);
            app.menu.Action(5, "Throw pizza right", "Land the box within three metres of the customer.",
                            running);
            app.menu.Action(6, "Refill boxes / renew shift", "Return to your starting shop to restock.",
                            running);
            app.menu.Action(7, "End shift", "Removes only this mod's customers, boxes and vehicle.",
                            running);
        }
        app.End();
        int a = int(app.menu.Activated());
        if (a == 2) {
            Route(Shop(store));
            Notice("Pizza shop waypoint set");
        }
        if (a == 3 && Start())
            app.menu.Close();
        if (a == 4)
            Throw(true);
        if (a == 5)
            Throw(false);
        if (a == 7) {
            Stop();
            Notice("Delivery shift cancelled");
        }
        if (running) {
            uint32_t now = Now();
            if (Due(now, endAt) || !Exists(bike) || ENTITY::IS_ENTITY_DEAD(PedSelf(), false)) {
                Stop();
                Notice("Delivery shift ended");
            } else {
                if (!app.Open()) {
                    if (Car() == bike) {
                        PAD::DISABLE_CONTROL_ACTION(0, 81, true);
                        PAD::DISABLE_CONTROL_ACTION(0, 82, true);
                        if (Press(81))
                            Throw(true);
                        if (Press(82))
                            Throw(false);
                    }
                }
                if (a == 6) {
                    if (Dist2(ENTITY::GET_ENTITY_COORDS(PedSelf(), true), Shop(shop)) < 100) {
                        boxes = 5;
                        endAt = now + 300000;
                        Notice("Five boxes restocked");
                    } else
                        Notice("Return to the starting shop to restock");
                }
                if (box) {
                    Vector3 p = ENTITY::GET_ENTITY_COORDS(box, true);
                    for (int i = 0; i < 4; ++i)
                        if (Exists(customers[i]) &&
                            Dist2(p, ENTITY::GET_ENTITY_COORDS(customers[i], true)) < 9) {
                            Delete(customers[i]);
                            Unpin(pins[i]);
                            Pay(MISC::GET_RANDOM_INT_IN_RANGE(10, 31));
                            ++delivered;
                            Notice("Pizza delivered");
                            Report("pizza_delivered");
                            Delete(box);
                            break;
                        }
                    if (box && Due(now, thrownAt + 3000))
                        Delete(box);
                }
                // Customers can disappear or die without receiving a box.
                // Clear their stale blips so the job cannot point at ghosts.
                for (int i = 0; i < 4; ++i)
                    if (customers[i] &&
                        (!Exists(customers[i]) || ENTITY::IS_ENTITY_DEAD(customers[i], false))) {
                        Delete(customers[i]);
                        Unpin(pins[i]);
                    }
                unsigned remaining = 0;
                for (Ped c : customers)
                    if (Exists(c))
                        ++remaining;
                if (!remaining) {
                    const bool complete = delivered == expected;
                    Stop();
                    Notice(complete ? "All deliveries complete"
                                    : "Shift ended: remaining customers were lost");
                    Report(complete ? "shift_complete" : "customers_lost");
                }
            }
        }
        WAIT(0);
    }
}
SHV_NX_PLUGIN_WITH_STOP("pizzaboy-nx", ScriptMain, Stop)
