#include "MesholaApp.h"
#include <TactilityCpp/App.h>

extern "C" {

/**
 * Entry point for Meshola - Multi-protocol mesh messaging
 * 
 * This is called when the app is loaded from SD card or flash.
 */
int main(int argc, char* argv[]) {
    registerApp<meshola::MesholaApp>();
    return 0;
}

}
