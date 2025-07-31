import ModuleWithComponent

MyComponentFromCpp {
    myProperty: 123
    function readMyProperty() { return myProperty; }
    Item { x: 42; }
    function callSignals() { someSignal(); mySlot(); myInvokable(); }
    gadget.myProperty: 34
    gadget { myProperty2: 35 }
}
