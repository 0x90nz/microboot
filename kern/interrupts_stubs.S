# I'll admit, this is quite a hacky solution, but it does enable us to not
# have massive amounts of repeated code in the interrupts C file.
#
# Basically we exploit the placement of data vs text segments to create
# an array of addresses to the interrup handlers, so we can just loop through
# them instead to needing to explicitly refer to each in the C file

.data
.global interrupts_stubs
interrupts_stubs:

.macro IRQ num exnum
.text
.global irq\num
irq\num\():
    cli
    pushl   $0
    pushl   $\exnum
    jmp     call_int_handle
.data
.long irq\num
.endm

.macro EXC num
.text
.global exception\num
exception\num\():
    cli
    pushl   $0
    pushl   $\num
    jmp     call_int_handle
.data
.long exception\num
.endm

.macro EXC_ERR num
.text
.global exception\num
exception\num\():
    cli
    pushl   $\num
    jmp     call_int_handle
.data
.long exception\num
.endm

.text
.extern interrupts_handle_int
call_int_handle:
    call    interrupts_handle_int
    add     $8, %esp
    sti
    iret

EXC 0
EXC 1
EXC 2
EXC 3
EXC 4
EXC 5
EXC 6
EXC 7
EXC_ERR 8
EXC 9
EXC_ERR 10
EXC_ERR 11
EXC_ERR 12
EXC_ERR 13
EXC_ERR 14
EXC 15
EXC 16
EXC_ERR 17
EXC 18
EXC 19
EXC 20
EXC_ERR 21
EXC 22
EXC 23
EXC 24
EXC 25
EXC 26
EXC 27
EXC 28
EXC 29
EXC_ERR 30
EXC 31

IRQ 0, 32
IRQ 1, 33
IRQ 2, 34
IRQ 3, 35
IRQ 4, 36
IRQ 5, 37
IRQ 6, 38
IRQ 7, 39
IRQ 8, 40
IRQ 9, 41
IRQ 10, 42
IRQ 11, 43
IRQ 12, 44
IRQ 13, 45
IRQ 14, 46
IRQ 15, 47

EXC 48
EXC 49
EXC 50
EXC 51
EXC 52
EXC 53
EXC 54
EXC 55
EXC 56
EXC 57
EXC 58
EXC 59
EXC 60
EXC 61
EXC 62
EXC 63
EXC 64
EXC 65
EXC 66
EXC 67
EXC 68
EXC 69
EXC 70
EXC 71
EXC 72
EXC 73
EXC 74
EXC 75
EXC 76
EXC 77
EXC 78
EXC 79
EXC 80
EXC 81
EXC 82
EXC 83
EXC 84
EXC 85
EXC 86
EXC 87
EXC 88
EXC 89
EXC 90
EXC 91
EXC 92
EXC 93
EXC 94
EXC 95
EXC 96
EXC 97
EXC 98
EXC 99
EXC 100
EXC 101
EXC 102 
EXC 103
EXC 104
EXC 105
EXC 106
EXC 107
EXC 108
EXC 109
EXC 110
EXC 111
EXC 112
EXC 113
EXC 114
EXC 115
EXC 116
EXC 117
EXC 118
EXC 119
EXC 120
EXC 121
EXC 122
EXC 123
EXC 124
EXC 125
EXC 126
EXC 127
EXC 128
EXC 129
EXC 130
EXC 131
EXC 132
EXC 133
EXC 134
EXC 135
EXC 136
EXC 137
EXC 138
EXC 139
EXC 140
EXC 141
EXC 142
EXC 143
EXC 144
EXC 145
EXC 146
EXC 147
EXC 148
EXC 149
EXC 150
EXC 151
EXC 152
EXC 153
EXC 154
EXC 155
EXC 156
EXC 157
EXC 158
EXC 159
EXC 160
EXC 161
EXC 162
EXC 163
EXC 164
EXC 165
EXC 166
EXC 167
EXC 168
EXC 169
EXC 170
EXC 171
EXC 172
EXC 173
EXC 174
EXC 175
EXC 176
EXC 177
EXC 178
EXC 179
EXC 180
EXC 181
EXC 182
EXC 183
EXC 184
EXC 185
EXC 186
EXC 187
EXC 188
EXC 189
EXC 190
EXC 191
EXC 192
EXC 193
EXC 194
EXC 195
EXC 196
EXC 197
EXC 198
EXC 199
EXC 200
EXC 201
EXC 202
EXC 203
EXC 204
EXC 205
EXC 206
EXC 207
EXC 208
EXC 209
EXC 210
EXC 211
EXC 212
EXC 213
EXC 214
EXC 215
EXC 216
EXC 217
EXC 218
EXC 219
EXC 220
EXC 221
EXC 222
EXC 223
EXC 224
EXC 225
EXC 226
EXC 227
EXC 228
EXC 229
EXC 230
EXC 231
EXC 232
EXC 233
EXC 234
EXC 235
EXC 236
EXC 237
EXC 238
EXC 239
EXC 240
EXC 241
EXC 242
EXC 243
EXC 244
EXC 245
EXC 246
EXC 247
EXC 248
EXC 249
EXC 250
EXC 251
EXC 252
EXC 253
EXC 254
EXC 255