#include "server.h"

int main() {
    Server myRedis(6739);
    if(myRedis.start()) {
        myRedis.run();
    }
    return 0;
}