#pragma once

#include "raiu/types.h"
#include "raiu/string.h"
#include "metadata.h"

i32 Link(ProgramContext *context, const String *rootpath);
i32 Execute(ProgramContext *context);

void Unlink(ProgramContext *context);

i32 Run(const String *rootpath);