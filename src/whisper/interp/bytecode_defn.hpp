#ifndef WHISPER__INTERP__BYTECODE_DEFN_HPP
#define WHISPER__INTERP__BYTECODE_DEFN_HPP


namespace Whisper {
namespace Interp {

// Macro iterating over all heap types.
#define WHISPER_BYTECODE_SEC0_OPS(_)                                \
/* Name       Format  Section  PopPush          Flags             */\
_(Section1,     E,      -1,     0,0,            OPF_SectionPrefix  )\
\
_(Nop,          E,      0,      0,0,            OPF_None           )\
_(Pop,          E,      0,      1,0,            OPF_None           )\
_(Stop,         E,      0,      0,0,            OPF_None           )\
\
_(PushInt8,     I1,     0,      0,1,            OPF_None           )\
_(PushInt16,    I2,     0,      0,1,            OPF_None           )\
_(PushInt24,    I3,     0,      0,1,            OPF_None           )\
_(PushInt32,    I4,     0,      0,1,            OPF_None           )\
_(Push,         V,      0,      0,1,            OPF_None           )\
\
_(Ret_S,        E,      0,      1,0,            OPF_Control        )\
_(Ret_V,        V,      0,      0,0,            OPF_Control        )\
\
_(Add_SSS,      E,      0,      2,1,            OPF_None           )\
_(Add_SSV,      V,      0,      2,0,            OPF_None           )\
_(Add_SVS,      V,      0,      1,1,            OPF_None           )\
_(Add_SVV,      VV,     0,      1,0,            OPF_None           )\
_(Add_VSS,      V,      0,      1,1,            OPF_None           )\
_(Add_VSV,      VV,     0,      1,0,            OPF_None           )\
_(Add_VVS,      VV,     0,      0,1,            OPF_None           )\
_(Add_VVV,      VVV,    0,      0,0,            OPF_None           )\
\
_(Sub_SSS,      E,      0,      2,1,            OPF_None           )\
_(Sub_SSV,      V,      0,      2,0,            OPF_None           )\
_(Sub_SVS,      V,      0,      1,1,            OPF_None           )\
_(Sub_SVV,      VV,     0,      1,0,            OPF_None           )\
_(Sub_VSS,      V,      0,      1,1,            OPF_None           )\
_(Sub_VSV,      VV,     0,      1,0,            OPF_None           )\
_(Sub_VVS,      VV,     0,      0,1,            OPF_None           )\
_(Sub_VVV,      VVV,    0,      0,0,            OPF_None           )\
\
_(Mul_SSS,      E,      0,      2,1,            OPF_None           )\
_(Mul_SSV,      V,      0,      2,0,            OPF_None           )\
_(Mul_SVS,      V,      0,      1,1,            OPF_None           )\
_(Mul_SVV,      VV,     0,      1,0,            OPF_None           )\
_(Mul_VSS,      V,      0,      1,1,            OPF_None           )\
_(Mul_VSV,      VV,     0,      1,0,            OPF_None           )\
_(Mul_VVS,      VV,     0,      0,1,            OPF_None           )\
_(Mul_VVV,      VVV,    0,      0,0,            OPF_None           )\
\
_(Div_SSS,      E,      0,      2,1,            OPF_None           )\
_(Div_SSV,      V,      0,      2,0,            OPF_None           )\
_(Div_SVS,      V,      0,      1,1,            OPF_None           )\
_(Div_SVV,      VV,     0,      1,0,            OPF_None           )\
_(Div_VSS,      V,      0,      1,1,            OPF_None           )\
_(Div_VSV,      VV,     0,      1,0,            OPF_None           )\
_(Div_VVS,      VV,     0,      0,1,            OPF_None           )\
_(Div_VVV,      VVV,    0,      0,0,            OPF_None           )\
\
_(Mod_SSS,      E,      0,      2,1,            OPF_None           )\
_(Mod_SSV,      V,      0,      2,0,            OPF_None           )\
_(Mod_SVS,      V,      0,      1,1,            OPF_None           )\
_(Mod_SVV,      VV,     0,      1,0,            OPF_None           )\
_(Mod_VSS,      V,      0,      1,1,            OPF_None           )\
_(Mod_VSV,      VV,     0,      1,0,            OPF_None           )\
_(Mod_VVS,      VV,     0,      0,1,            OPF_None           )\
_(Mod_VVV,      VVV,    0,      0,0,            OPF_None           )\
\
_(Neg_SS,       E,      0,      1,1,            OPF_None           )\
_(Neg_SV,       V,      0,      1,0,            OPF_None           )\
_(Neg_VS,       V,      0,      0,1,            OPF_None           )\
_(Neg_VV,       VV,     0,      0,0,            OPF_None           )


#define WHISPER_BYTECODE_MAX_SECTION    (1)


} // namespace Interp
} // namespace Whisper

#endif // WHISPER__INTERP__BYTECODE_DEFN_HPP
