description("Tests that DFG operations with floating-point arguments don't crash as in bug 115585.");

function arrayPush(array, value) {
    array.push(value);
}

function arrayPushBeyondBounds(array, value) {
    var length = array.length;
    array[length] = value;
}

function arrayPushBeyondBoundsStrict(array, value) {
    "use strict";
    var length = array.length;
    array[length] = value;
}

function testArrayFunc(func, value) {
    var n = 200;
    var array = [];
    for (var i = 0; i < n; ++i) {
        func(array, value);
    }
    var sum = 0;
    for (var i = 0; i < n; ++i) {
        sum += array[i];
    }
    return sum;
}

shouldBe("testArrayFunc(arrayPush, 1.25)", "250");
shouldBe("testArrayFunc(arrayPushBeyondBounds, 1.25)", "250");
shouldBe("testArrayFunc(arrayPushBeyondBoundsStrict, 1.25)", "250");
