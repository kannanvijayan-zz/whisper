#ifndef WHISPER__INTERP__BYTECODE_DEFN_HPP
#define WHISPER__INTERP__BYTECODE_DEFN_HPP


namespace Whisper {
namespace Interp {

// Macro iterating over all heap types.
#define WHISPER_BYTECODE_SEC0_OPS(_)                                \
/* Name         Format  Section Flags                             */\
_(Section1,     E,      -1,     OPF_SectionPrefix                  )\
\
_(Nop,          E,      0,      OPF_None                           )\
\
_(PushInt8,     I1,     0,      OPF_None                           )\
_(PushInt16,    I2,     0,      OPF_None                           )\
_(PushInt24,    I3,     0,      OPF_None                           )\
_(PushInt32,    I4,     0,      OPF_None                           )\
\
_(Ret_S,        E,      0,      OPF_Control                        )\
_(Ret_V,        V,      0,      OPF_Control                        )\
\
_(If_S,         E,      0,      OPF_Control                        )\
_(If_V,         V,      0,      OPF_Control                        )\
_(IfNot_S,      E,      0,      OPF_Control                        )\
_(IfNot_V,      V,      0,      OPF_Control                        )\
\
_(Add_SSS,      E,      0,      OPF_None                           )\
_(Add_SSV,      V,      0,      OPF_None                           )\
_(Add_SVS,      V,      0,      OPF_None                           )\
_(Add_SVV,      VV,     0,      OPF_None                           )\
_(Add_VSS,      V,      0,      OPF_None                           )\
_(Add_VSV,      VV,     0,      OPF_None                           )\
_(Add_VVS,      VV,     0,      OPF_None                           )\
_(Add_VVV,      VVV,    0,      OPF_None                           )\


} // namespace Interp
} // namespace Whisper

#endif // WHISPER__INTERP__BYTECODE_DEFN_HPP
