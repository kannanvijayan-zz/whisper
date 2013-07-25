#ifndef WHISPER__VM__CLASS_HPP
#define WHISPER__VM__CLASS_HPP

#include "common.hpp"
#include "debug.hpp"
#include "runtime.hpp"

#include <limits>
#include <algorithm>

namespace Whisper {
namespace VM {

class Object;
class PropertyKey;
class PropertyDescriptor;


//
// A Class describes the overarching behaviour of any object that
// belongs to it.
//
// It specifies the following basic attributes about an object:
//  * The number of hidden slots in the object.
//  * Whether property accesses have a pre-emptive trap.
//  * Whether property accesses have a fallback trap.
//
// It also provides any necessary handlers for essential internal method
// implementations which are overridden from the "default" handling.
//
class Class
{
  private:
    // Up to 16 hidden slots.
    uint32_t numHiddenSlots_ : 4;

    // Flags.
    uint32_t handleGetInheritance_ : 1;
    uint32_t handleSetInheritance_ : 1;
    uint32_t handleIsExtensible_ : 1;
    uint32_t handlePreventExtensions_ : 1;

    uint32_t trapHasOwnProperty_ : 1;
    uint32_t fallbackHasOwnProperty_ : 1;

    uint32_t trapGetOwnProperty_ : 1;
    uint32_t fallbackGetOwnProperty_ : 1;

    uint32_t trapHasProperty_ : 1;
    uint32_t fallbackHasProperty_ : 1;

    uint32_t trapGet_ : 1;
    uint32_t fallbackGet_ : 1;

    uint32_t trapSet_ : 1;
    uint32_t fallbackSet_ : 1;

    uint32_t trapInvoke_ : 1;
    uint32_t fallbackInvoke_ : 1;

    uint32_t trapDelete_ : 1;
    uint32_t fallbackDelete_ : 1;

    uint32_t trapDefineOwnProperty_ : 1;
    uint32_t fallbackDefineOwnProperty_ : 1;

    uint32_t handleEnumerate_ : 1;
    uint32_t handleOwnPropertyKeys_ : 1;
    uint32_t handleCall_ : 1;
    uint32_t handleConstruct_ : 1;

    enum TrapResult { Error=0, Skip, Hit };

    // Handlers types.
    typedef bool (*GetInheritanceHandler)(RunContext *cx, Object **result,
                                          Object *obj);
    GetInheritanceHandler *getInheritance_;


    typedef bool (*SetInheritanceHandler)(RunContext *cx, bool *result,
                                          Object *obj, Object *anc);
    SetInheritanceHandler *setInheritance_;


    typedef bool (*IsExtensibleHandler)(RunContext *cx, bool *result,
                                        Object *obj);
    IsExtensibleHandler *isExtensible_;


    typedef bool (*PreventExtensionsHandler)(RunContext *cx, bool *result,
                                             Object *obj);
    PreventExtensionsHandler *preventExtensions_;


    typedef TrapResult (*HasOwnPropertyTrap)(RunContext *cx, bool *result,
                                             Object *obj, PropertyKey *key);
    HasOwnPropertyTrap *hasOwnPropertyTrap_;
    HasOwnPropertyTrap *hasOwnPropertyFallback_;


    typedef TrapResult (*GetOwnPropertyTrap)(RunContext *cx,
                                             PropertyDescriptor *result,
                                             Object *obj, PropertyKey *key);
    GetOwnPropertyTrap *getOwnPropertyTrap_;
    GetOwnPropertyTrap *getOwnPropertyFallback_;


    typedef TrapResult (*GetPropertyTrap)(RunContext *cx, bool *result,
                                          Object *obj, PropertyKey *key);
    GetPropertyTrap *getPropertyTrap_;
    GetPropertyTrap *getPropertyFallback_;


    typedef TrapResult (*GetTrap)(RunContext *cx, bool *result,
                                  Object *obj, PropertyKey *key,
                                  Value receiver);
    GetTrap *getTrap_;
    GetTrap *getFallback_;

    typedef TrapResult (*SetTrap)(RunContext *cx, bool *result,
                                  Object *obj, PropertyKey *key,
                                  Value value, Value receiver);
    SetTrap *setTrap_;
    SetTrap *setFallback_;

    typedef TrapResult (*InvokeTrap)(RunContext *cx, Value *result,
                                     Object *obj, PropertyKey *key,
                                     Value *args, Value receiver);
    InvokeTrap *invokeTrap_;
    InvokeTrap *invokeFallback_;

    typedef TrapResult (*DeleteTrap)(RunContext *cx, bool *result,
                                     Object *obj);
    DeleteTrap *deleteTrap_;
    DeleteTrap *deleteFallback_;

    typedef TrapResult (*DefineOwnPropertyTrap)(RunContext *cx, bool *result,
                                                Object *obj, PropertyKey *key);
    DefineOwnPropertyTrap *defineOwnPropertyTrap_;
    DefineOwnPropertyTrap *defineOwnPropertyFallback_;

    typedef bool (*EnumerateHandler)(RunContext *cx, Object **result,
                                     Object *obj);
    EnumerateHandler *enumerateHandler_;

    typedef bool (*OwnPropertyKeysHandler)(RunContext *cx, Object **result,
                                           Object *obj);
    OwnPropertyKeysHandler *ownPropertyKeysHandler_;

    typedef bool (*CallHandler)(RunContext *cx, Value *result,
                                Value thisVal, Value *args);
    CallHandler *callHandler_;

    typedef bool (*ConstructHandler)(RunContext *cx, Value *result,
                                     Value *args);
    ConstructHandler *constructHandler_;
};


} // namespace VM
} // namespace Whisper

#endif // WHISPER__VM__CLASS_HPP
