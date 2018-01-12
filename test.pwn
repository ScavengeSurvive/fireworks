#include "fireworks.inc"

#include <zcmd>
#include <test-boilerplate>
#include <YSI\y_testing>

new
    FireworkSequence:fwkSequence1;

main() {
    logger_debug("fireworks", true);

    new FireworkExplosion:a = DefineFireworkExplosionType(
        {345}, 1,
        60.0, 20.0, 12);

    new FireworkExplosion:b = DefineFireworkExplosionType(
        {345, 18724}, 2,
        45.0, 50.0, 6);

    new FireworkExplosion:c = DefineFireworkExplosionType(
        {345, 18688, 18670}, 3,
        45.0, 6.0, 16);

    fwkSequence1 = DefineFireworkExplosionSequence(a, b, c);

    // CreateFirework(fwkSequence1, 302.0, 1809.0, 18.0);
}

public OnPlayerSpawn(playerid) {
    SetWorldTime(0);
    CreateDynamicObject(1337, 302.0, 1809.0, 18.0, 0.0, 0.0, 0.0);
    defer test1();
}

timer test1[3000]() {
    CreateFirework(fwkSequence1, 302.0, 1809.0, 18.0);
}

CMD:fireworks(playerid, params[]) {
    defer test1();
    return 1;
}