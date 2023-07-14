// MIT License
// Copyright (c) 2023 Lauri Lorenzo Fiestas
// https://github.com/PrinssiFiestas/libGPC/blob/main/LICENSE.md
 
#ifndef GPC_ERRORMSGS_H
#define GPC_ERRORMSGS_H

#define GPC_EMSG_INTERNAL 			"GPC INTERNAL ERROR: "
#define GPC_EMSG_NULL_ARG(arg,f)	#arg " is NULL in " #f "()"
#define GPC_EMSG_NULL_OWNER(f)		"Owner is NULL in " #f "(). Every object requires an owner!"
#define GPC_EMSG_OBJ_NO_OWNER(f)	"Object passed to " #f "() has no owner. "	\
									"Every object requires an owner!"
#define GPC_EMSG_0ALLOC(f)			"Trying to allocate 0 bytes in " #f "()"
#define GPC_EMSG_OVERALLOC(f)		"Trying to allocate over PTRDIFF_MAX bytes in " #f "(). "\
									"Is the input positive?"
#define GPC_EMSG_NULL_PASSED(f)		"Passed NULL to" #f "()"

#endif // GPC_ERRORMSGS_H
