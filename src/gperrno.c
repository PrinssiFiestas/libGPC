// MIT License
// Copyright (c) 2023 Lauri Lorenzo Fiestas
// https://github.com/PrinssiFiestas/libGPC/blob/main/LICENSE.md

#include <gpc/gperrno.h>
#include <gpc/gputils.h>
#include <gpc/gpthread.h>
#include <stdint.h>
#ifdef GP_TARGET_OS_WINDOWS
#include <windows.h>
#endif // GP_TARGET_OS_WINDOWS

#ifdef GP_TARGET_OS_WINDOWS
static char* gp_s_doserrno_str(int doserr)
{
}
#endif // GP_TARGET_OS_WINDOWS

char* gp_errno_str(GPString* optional_dest, const GPErrno* optional_errno)
{
    int errnum = optional_errno != NULL ? optional_errno->std : errno;
    #ifdef GP_TARGET_OS_WINDOWS
    char* str;
    int doserr = optional_errno != NULL ? optional_errno->dos : _doserrno;
    if (doserr != 0) {
        if ((str = gp_s_doserrno_str(doserr)) == NULL)
            str = strerror(errnum);
    } else
        str = strerror(errnum);
    #else
    char* str = strerror(errnum);
    #endif

    if (optional_dest == NULL)
        return str;

    #if 0 // TODO string not implemented at the time of writing.
    size_t len = strlen(str);
    // TODO gp_str_reserve_safe() not implemented at the time of writing.
    bool reserved = gp_str_reserve_safe(optional_dest, len);
    if ( ! reserved) { // truncate
        len = gp_str_capacity(*optional_dest);
        if (gp_likely(len >= 3)) {
            memcpy(*optional_dest, str, len - 3);
            memcpy(*optional_dest + len - 3, "...", sizeof"...");
        } else if (len > 0) {
            memcpy(*optional_dest, "?", sizeof"?");
            len = 1;
        }
        // TODO should we return str here?
    } else
        memcpy(*optional_dest, str, len + sizeof"");
    gp_str_set(*optional_dest)->length = len;
    #endif
    return (char*)*optional_dest;
}

#ifdef GP_TARGET_OS_WINDOWS
static uint8_t gp_s_error_table[] =
{
    0,
    ENOSYS,       /* ERROR_INVALID_FUNCTION                         1 */
    ENOENT,       /* ERROR_FILE_NOT_FOUND                           2 */
    ENOENT,       /* ERROR_PATH_NOT_FOUND                           3 */
    EMFILE,       /* ERROR_TOO_MANY_OPEN_FILES                      4 */
    EACCES,       /* ERROR_ACCESS_DENIED                            5 */
    EBADF,        /* ERROR_INVALID_HANDLE                           6 */
    ENOMEM,       /* ERROR_ARENA_TRASHED                            7 */
    ENOMEM,       /* ERROR_NOT_ENOUGH_MEMORY                        8 */
    EFAULT,       /* ERROR_INVALID_BLOCK                            9 */
    E2BIG,        /* ERROR_BAD_ENVIRONMENT                         10 */
    ENOEXEC,      /* ERROR_BAD_FORMAT                              11 */
    EACCES,       /* ERROR_INVALID_ACCESS                          12 */
    EINVAL,       /* ERROR_INVALID_DATA                            13 */
    ENOMEM,       /* ERROR_OUT_OF_MEMORY                           14 */
    ENOENT,       /* ERROR_INVALID_DRIVE                           15 */
    EACCES,       /* ERROR_CURRENT_DIRECTORY                       16 */
    EXDEV,        /* ERROR_NOT_SAME_DEVICE                         17 */
    ENOENT,       /* ERROR_NO_MORE_FILES                           18 */
    EROFS,        /* ERROR_WRITE_PROTECT                           19 */
    ENXIO,        /* ERROR_BAD_UNIT                                20 */
    EBUSY,        /* ERROR_NOT_READY                               21 */
    EIO,          /* ERROR_BAD_COMMAND                             22 */
    EIO,          /* ERROR_CRC                                     23 */
    EINVAL,       /* ERROR_BAD_LENGTH                              24 */
    EIO,          /* ERROR_SEEK                                    25 */
    EIO,          /* ERROR_NOT_DOS_DISK                            26 */
    ENXIO,        /* ERROR_SECTOR_NOT_FOUND                        27 */
    ENOSPC,       /* ERROR_OUT_OF_PAPER                            28 */
    EIO,          /* ERROR_WRITE_FAULT                             29 */
    EIO,          /* ERROR_READ_FAULT                              30 */
    EIO,          /* ERROR_GEN_FAILURE                             31 */
    EACCES,       /* ERROR_SHARING_VIOLATION                       32 */
    EACCES,       /* ERROR_LOCK_VIOLATION                          33 */
    ENXIO,        /* ERROR_WRONG_DISK                              34 */
    ENFILE,       /* ERROR_FCB_UNAVAILABLE                         35 */
    ENFILE,       /* ERROR_SHARING_BUFFER_EXCEEDED                 36 */
    -1,           /*                                               37 */
    ENODATA,      /* ERROR_HANDLE_EOF                              38 */
    ENOSPC,       /* ERROR_HANDLE_DISK_FULL                        39 */
    -1,           /*                                               40 */
    -1,           /*                                               41 */
    -1,           /*                                               42 */
    -1,           /*                                               43 */
    -1,           /*                                               44 */
    -1,           /*                                               45 */
    -1,           /*                                               46 */
    -1,           /*                                               47 */
    -1,           /*                                               48 */
    -1,           /*                                               49 */
    ENOTSUP,      /* ERROR_NOT_SUPPORTED                           50 */
    ECONNREFUSED, /* ERROR_REM_NOT_LIST                            51 */
    EADDRINUSE,   /* ERROR_DUP_NAME                                52 */
    ENOENT,       /* ERROR_BAD_NETPATH                             53 */
    EBUSY,        /* ERROR_NETWORK_BUSY                            54 */
    ENODEV,       /* ERROR_DEV_NOT_EXIST                           55 */
    EAGAIN,       /* ERROR_TOO_MANY_CMDS                           56 */
    EIO,          /* ERROR_ADAP_HDW_ERR                            57 */
    EIO,          /* ERROR_BAD_NET_RESP                            58 */
    EIO,          /* ERROR_UNEXP_NET_ERR                           59 */
    EINVAL,       /* ERROR_BAD_REM_ADAP                            60 */
    EFBIG,        /* ERROR_PRINTQ_FULL                             61 */
    ENOSPC,       /* ERROR_NO_SPOOL_SPACE                          62 */
    ENOENT,       /* ERROR_PRINT_CANCELLED                         63 */
    ENOENT,       /* ERROR_NETNAME_DELETED                         64 */
    EACCES,       /* ERROR_NETWORK_ACCESS_DENIED                   65 */
    ENODEV,       /* ERROR_BAD_DEV_TYPE                            66 */
    ENOENT,       /* ERROR_BAD_NET_NAME                            67 */
    EIO,          /* ERROR_TOO_MANY_NAMES                          68 */
    EIO,          /* ERROR_TOO_MANY_SESS                           69 */
    EAGAIN,       /* ERROR_SHARING_PAUSED                          70 */
    EAGAIN,       /* ERROR_REQ_NOT_ACCEP                           71 */
    EAGAIN,       /* ERROR_REDIR_PAUSED                            72 */
    -1,           /*                                               73 */
    -1,           /*                                               74 */
    -1,           /*                                               75 */
    -1,           /*                                               76 */
    -1,           /*                                               77 */
    -1,           /*                                               78 */
    -1,           /*                                               79 */
    EEXIST,       /* ERROR_FILE_EXISTS                             80 */
    -1,           /*                                               81 */
    EACCES,       /* ERROR_CANNOT_MAKE                             82 */
    EIO,          /* ERROR_FAIL_I24                                83 */
    ENOMEM,       /* ERROR_OUT_OF_STRUCTURES                       84 */
    EEXIST,       /* ERROR_ALREADY_ASSIGNED                        85 */
    EPERM,        /* ERROR_INVALID_PASSWORD                        86 */
    EINVAL,       /* ERROR_INVALID_PARAMETER                       87 */
    EIO,          /* ERROR_NET_WRITE_FAULT                         88 */
    EAGAIN,       /* ERROR_NO_PROC_SLOTS                           89 */
    -1,           /*                                               90 */
    -1,           /*                                               91 */
    -1,           /*                                               92 */
    -1,           /*                                               93 */
    -1,           /*                                               94 */
    -1,           /*                                               95 */
    -1,           /*                                               96 */
    -1,           /*                                               97 */
    -1,           /*                                               98 */
    -1,           /*                                               99 */
    ENOSPC,       /* ERROR_TOO_MANY_SEMAPHORES                    100 */
    EBUSY,        /* ERROR_EXCL_SEM_ALREADY_OWNED                 101 */
    EBUSY,        /* ERROR_SEM_IS_SET                             102 */
    EAGAIN,       /* ERROR_TOO_MANY_SEM_REQUESTS                  103 */
    EINVAL,       /* ERROR_INVALID_AT_INTERRUPT_TIME              104 */
    EOWNERDEAD,   /* ERROR_SEM_OWNER_DIED                         105 */
    ENOSPC,       /* ERROR_SEM_USER_LIMIT                         106 */
    EXDEV,        /* ERROR_DISK_CHANGE                            107 */
    EAGAIN,       /* ERROR_DRIVE_LOCKED                           108 */
    EPIPE,        /* ERROR_BROKEN_PIPE                            109 */
    ENOENT,       /* ERROR_OPEN_FAILED                            110 */
    ENAMETOOLONG, /* ERROR_BUFFER_OVERFLOW                        111 */
    ENOSPC,       /* ERROR_DISK_FULL                              112 */
    EMFILE,       /* ERROR_NO_MORE_SEARCH_HANDLES                 113 */
    EBADF,        /* ERROR_INVALID_TARGET_HANDLE                  114 */
    EFAULT,       /* ERROR_PROTECTION_VIOLATION                   115 */
    -1,           /*                                              116 */
    EINVAL,       /* ERROR_INVALID_CATEGORY                       117 */
    EINVAL,       /* ERROR_INVALID_VERIFY_SWITCH                  118 */
    ENXIO,        /* ERROR_BAD_DRIVER_LEVEL                       119 */
    ENOSYS,       /* ERROR_CALL_NOT_IMPLEMENTED                   120 */
    ETIMEDOUT,    /* ERROR_SEM_TIMEOUT                            121 */
    EINVAL,       /* ERROR_INSUFFICIENT_BUFFER                    122 */
    ENOENT,       /* ERROR_INVALID_NAME                           123 */
    ENOTSUP,      /* ERROR_INVALID_LEVEL                          124 */
    ENODATA,      /* ERROR_NO_VOLUME_LABEL                        125 */
    ENOENT,       /* ERROR_MOD_NOT_FOUND                          126 */
    ENOSYS,       /* ERROR_PROC_NOT_FOUND                         127 */
    ECHILD,       /* ERROR_WAIT_NO_CHILDREN                       128 */
    ECHILD,       /* ERROR_CHILD_NOT_COMPLETE                     129 */
    EBADF,        /* ERROR_DIRECT_ACCESS_HANDLE                   130 */
    EINVAL,       /* ERROR_NEGATIVE_SEEK                          131 */
    ESPIPE,       /* ERROR_SEEK_ON_DEVICE                         132 */
    EINVAL,       /* ERROR_IS_JOIN_TARGET                         133 */
    EINVAL,       /* ERROR_IS_JOINED                              134 */
    EINVAL,       /* ERROR_IS_SUBSTED                             135 */
    EINVAL,       /* ERROR_NOT_JOINED                             136 */
    EINVAL,       /* ERROR_NOT_SUBSTED                            137 */
    EINVAL,       /* ERROR_JOIN_TO_JOIN                           138 */
    EINVAL,       /* ERROR_SUBST_TO_SUBST                         139 */
    EINVAL,       /* ERROR_JOIN_TO_SUBST                          140 */
    EINVAL,       /* ERROR_SUBST_TO_JOIN                          141 */
    EAGAIN,       /* ERROR_BUSY_DRIVE                             142 */
    EINVAL,       /* ERROR_SAME_DRIVE                             143 */
    ENOTDIR,      /* ERROR_DIR_NOT_ROOT                           144 */
    ENOTEMPTY,    /* ERROR_DIR_NOT_EMPTY                          145 */
    EINVAL,       /* ERROR_IS_SUBST_PATH                          146 */
    EINVAL,       /* ERROR_IS_JOIN_PATH                           147 */
    EBUSY,        /* ERROR_PATH_BUSY                              148 */
    EINVAL,       /* ERROR_IS_SUBST_TARGET                        149 */
    EPERM,        /* ERROR_SYSTEM_TRACE                           150 */
    EINVAL,       /* ERROR_INVALID_EVENT_COUNT                    151 */
    EAGAIN,       /* ERROR_TOO_MANY_MUXWAITERS                    152 */
    EINVAL,       /* ERROR_INVALID_LIST_FORMAT                    153 */
    ENAMETOOLONG, /* ERROR_LABEL_TOO_LONG                         154 */
    EAGAIN,       /* ERROR_TOO_MANY_TCBS                          155 */
    EINTR,        /* ERROR_SIGNAL_REFUSED                         156 */
    EINVAL,       /* ERROR_DISCARDED                              157 */
    EACCES,       /* ERROR_NOT_LOCKED                             158 */
    EINVAL,       /* ERROR_BAD_THREADID_ADDR                      159 */
    EINVAL,       /* ERROR_BAD_ARGUMENTS                          160 */
    ENOENT,       /* ERROR_BAD_PATHNAME                           161 */
    EINPROGRESS,  /* ERROR_SIGNAL_PENDING                         162 */
    EAGAIN,       /* ERROR_MAX_THRDS_REACHED                      163 */
    -1,           /*                                              164 */
    -1,           /*                                              165 */
    -1,           /*                                              166 */
    EACCES,       /* ERROR_LOCK_FAILED                            167 */
    -1,           /*                                              168 */
    -1,           /*                                              169 */
    EBUSY,        /* ERROR_BUSY                                   170 */
    EINPROGRESS,  /* ERROR_DEVICE_SUPPORT_IN_PROGRESS             171 */
    -1,           /*                                              172 */
    EINVAL,       /* ERROR_CANCEL_VIOLATION                       173 */
    ENOTSUP,      /* ERROR_ATOMIC_LOCKS_NOT_SUPPORTED             174 */
    -1,           /*                                              175 */
    -1,           /*                                              176 */
    -1,           /*                                              177 */
    -1,           /*                                              178 */
    -1,           /*                                              179 */
    EINVAL,       /* ERROR_INVALID_SEGMENT_NUMBER                 180 */
    -1,           /*                                              181 */
    EINVAL,       /* ERROR_INVALID_ORDINAL                        182 */
    EEXIST,       /* ERROR_ALREADY_EXISTS                         183 */
    ECHILD,       /* ERROR_NO_CHILD_PROCESS                       184 */
    -1,           /*                                              185 */
    EINVAL,       /* ERROR_INVALID_FLAG_NUMBER                    186 */
    ENOENT,       /* ERROR_SEM_NOT_FOUND                          187 */
    ENOEXEC,      /* ERROR_INVALID_STARTING_CODESEG               188 */
    ENOEXEC,      /* ERROR_INVALID_STACKSEG                       189 */
    ENOEXEC,      /* ERROR_INVALID_MODULETYPE                     190 */
    ENOEXEC,      /* ERROR_INVALID_EXE_SIGNATURE                  191 */
    ENOEXEC,      /* ERROR_EXE_MARKED_INVALID                     192 */
    ENOEXEC,      /* ERROR_BAD_EXE_FORMAT                         193 */
    ENOEXEC,      /* ERROR_ITERATED_DATA_EXCEEDS_64k              194 */
    ENOEXEC,      /* ERROR_INVALID_MINALLOCSIZE                   195 */
    EPERM,        /* ERROR_DYNLINK_FROM_INVALID_RING              196 */
    EPERM,        /* ERROR_IOPL_NOT_ENABLED                       197 */
    ENOEXEC,      /* ERROR_INVALID_SEGDPL                         198 */
    ENOEXEC,      /* ERROR_AUTODATASEG_EXCEEDS_64k                199 */
    ENOEXEC,      /* ERROR_RING2SEG_MUST_BE_MOVABLE               200 */
    ENOEXEC,      /* ERROR_RELOC_CHAIN_XEEDS_SEGLIM               201 */
    ENOEXEC,      /* ERROR_INFLOOP_IN_RELOC_CHAIN                 202 */
    EINVAL,       /* ERROR_ENVVAR_NOT_FOUND                       203 */
    -1,           /*                                              204 */
    ENOENT,       /* ERROR_NO_SIGNAL_SENT                         205 */
    ENAMETOOLONG, /* ERROR_FILENAME_EXCED_RANGE                   206 */
    EBUSY,        /* ERROR_RING2_STACK_IN_USE                     207 */
    E2BIG,        /* ERROR_META_EXPANSION_TOO_LONG                208 */
    EINVAL,       /* ERROR_INVALID_SIGNAL_NUMBER                  209 */
    EPERM,        /* ERROR_THREAD_1_INACTIVE                      210 */
    -1,           /*                                              211 */
    EBUSY,        /* ERROR_LOCKED                                 212 */
    -1,           /*                                              213 */
    EMFILE,       /* ERROR_TOO_MANY_MODULES                       214 */
    EINVAL,       /* ERROR_NESTING_NOT_ALLOWED                    215 */
    ENOEXEC,      /* ERROR_EXE_MACHINE_TYPE_MISMATCH              216 */
    EACCES,       /* ERROR_EXE_CANNOT_MODIFY_SIGNED_BINARY        217 */
    EACCES,       /* ERROR_EXE_CANNOT_MODIFY_STRONG_SIGNED_BINARY 218 */
    -1,           /*                                              219 */
    EACCES,       /* ERROR_FILE_CHECKED_OUT                       220 */
    EACCES,       /* ERROR_CHECKOUT_REQUIRED                      221 */
    EACCES,       /* ERROR_BAD_FILE_TYPE                          222 */
    EFBIG,        /* ERROR_FILE_TOO_LARGE                         223 */
    EACCES,       /* ERROR_FORMS_AUTH_REQUIRED                    224 */
    EACCES,       /* ERROR_VIRUS_INFECTED                         225 */
    EACCES,       /* ERROR_VIRUS_DELETED                          226 */
    -1,           /*                                              227 */
    -1,           /*                                              228 */
    ENOTSUP,      /* ERROR_PIPE_LOCAL                             229 */
    EPIPE,        /* ERROR_BAD_PIPE                               230 */
    EAGAIN,       /* ERROR_PIPE_BUSY                              231 */
    EPIPE,        /* ERROR_NO_DATA                                232 */
    EPIPE,        /* ERROR_PIPE_NOT_CONNECTED                     233 */
    EMSGSIZE,     /* ERROR_MORE_DATA                              234 */
    -1,           /*                                              235 */
    -1,           /*                                              236 */
    -1,           /*                                              237 */
    -1,           /*                                              238 */
    -1,           /*                                              239 */
    ECONNRESET,   /* ERROR_VC_DISCONNECTED                        240 */
    -1,           /*                                              241 */
    -1,           /*                                              242 */
    -1,           /*                                              243 */
    -1,           /*                                              244 */
    -1,           /*                                              245 */
    -1,           /*                                              246 */
    -1,           /*                                              247 */
    -1,           /*                                              248 */
    -1,           /*                                              249 */
    -1,           /*                                              250 */
    -1,           /*                                              251 */
    -1,           /*                                              252 */
    -1,           /*                                              253 */
    EINVAL,       /* ERROR_INVALID_EA_NAME                        254 */
    EINVAL,       /* ERROR_EA_LIST_INCONSISTENT                   255 */
    -1,           /*                                              256 */
    -1,           /*                                              257 */
    ETIMEDOUT,    /* WAIT_TIMEOUT                                 258 */
    ENODATA,      /* ERROR_NO_MORE_ITEMS                          259 */
    -1,           /*                                              260 */
    -1,           /*                                              261 */
    -1,           /*                                              262 */
    -1,           /*                                              263 */
    -1,           /*                                              264 */
    -1,           /*                                              265 */
    ENOTSUP,      /* ERROR_CANNOT_COPY                            266 */
    ENOTDIR,      /* ERROR_DIRECTORY                              267 */
    -1,           /*                                              268 */
    -1,           /*                                              269 */
    -1,           /*                                              270 */
    -1,           /*                                              271 */
    -1,           /*                                              272 */
    -1,           /*                                              273 */
    -1,           /*                                              274 */
    ENOSPC,       /* ERROR_EAS_DIDNT_FIT                          275 */
    EIO,          /* ERROR_EA_FILE_CORRUPT                        276 */
    ENOSPC,       /* ERROR_EA_TABLE_FULL                          277 */
    EBADF,        /* ERROR_INVALID_EA_HANDLE                      278 */
    -1,           /*                                              279 */
    -1,           /*                                              280 */
    -1,           /*                                              281 */
    ENOTSUP,      /* ERROR_EAS_NOT_SUPPORTED                      282 */
    -1,           /*                                              283 */
    -1,           /*                                              284 */
    -1,           /*                                              285 */
    -1,           /*                                              286 */
    -1,           /*                                              287 */
    EPERM,        /* ERROR_NOT_OWNER                              288 */
    -1,           /*                                              289 */
    -1,           /*                                              290 */
    -1,           /*                                              291 */
    -1,           /*                                              292 */
    -1,           /*                                              293 */
    -1,           /*                                              294 */
    -1,           /*                                              295 */
    -1,           /*                                              296 */
    -1,           /*                                              297 */
    EOVERFLOW,    /* ERROR_TOO_MANY_POSTS                         298 */
    EFAULT,       /* ERROR_PARTIAL_COPY                           299 */
    EAGAIN,       /* ERROR_OPLOCK_NOT_GRANTED                     300 */
    EPROTO,       /* ERROR_INVALID_OPLOCK_PROTOCOL                301 */
    ENOSPC,       /* ERROR_DISK_TOO_FRAGMENTED                    302 */
    ENOENT,       /* ERROR_DELETE_PENDING                         303 */
    EPERM,        /* ERROR_INCOMPATIBLE_WITH_GLOBAL_SHORT_NAME_REGISTRY_SETTING 304 */
    ENOTSUP,      /* ERROR_SHORT_NAMES_NOT_ENABLED_ON_VOLUME      305 */
    EIO,          /* ERROR_SECURITY_STREAM_IS_INCONSISTENT        306 */
    EINVAL,       /* ERROR_INVALID_LOCK_RANGE                     307 */
    ENOSYS,       /* ERROR_IMAGE_SUBSYSTEM_NOT_PRESENT            308 */
    EEXIST,       /* ERROR_NOTIFICATION_GUID_ALREADY_DEFINED      309 */
    EFAULT,       /* ERROR_INVALID_EXCEPTION_HANDLER              310 */
    EINVAL,       /* ERROR_DUPLICATE_PRIVILEGES                   311 */
    ENODATA,      /* ERROR_NO_RANGES_PROCESSED                    312 */
    EACCES,       /* ERROR_NOT_ALLOWED_ON_SYSTEM_FILE             313 */
    ENOSPC,       /* ERROR_DISK_RESOURCES_EXHAUSTED               314 */
    EINVAL,       /* ERROR_INVALID_TOKEN                          315 */
    ENOTSUP,      /* ERROR_DEVICE_FEATURE_NOT_SUPPORTED           316 */
    ENOENT,       /* ERROR_MR_MID_NOT_FOUND                       317 */
    ENOENT,       /* ERROR_SCOPE_NOT_FOUND                        318 */
    ENOENT,       /* ERROR_UNDEFINED_SCOPE                        319 */
    EINVAL,       /* ERROR_INVALID_CAP                            320 */
    EHOSTUNREACH, /* ERROR_DEVICE_UNREACHABLE                     321 */
    ENOMEM,       /* ERROR_DEVICE_NO_RESOURCES                    322 */
    EIO,          /* ERROR_DATA_CHECKSUM_ERROR                    323 */
    EINVAL,       /* ERROR_INTERMIXED_KERNEL_EA_OPERATION         324 */
    -1,           /*                                              325 */
    ENOTSUP,      /* ERROR_FILE_LEVEL_TRIM_NOT_SUPPORTED          326 */
    EINVAL,       /* ERROR_OFFSET_ALIGNMENT_VIOLATION             327 */
    EINVAL,       /* ERROR_INVALID_FIELD_IN_PARAMETER_LIST        328 */
    EINPROGRESS,  /* ERROR_OPERATION_IN_PROGRESS                  329 */
    ENOENT,       /* ERROR_BAD_DEVICE_PATH                        330 */
    EMFILE,       /* ERROR_TOO_MANY_DESCRIPTORS                   331 */
    ENOTSUP,      /* ERROR_SCRUB_DATA_DISABLED                    332 */
    ENOTSUP,      /* ERROR_NOT_REDUNDANT_STORAGE                  333 */
    ENOTSUP,      /* ERROR_RESIDENT_FILE_NOT_SUPPORTED            334 */
    ENOTSUP,      /* ERROR_COMPRESSED_FILE_NOT_SUPPORTED          335 */
    ENOTSUP,      /* ERROR_DIRECTORY_NOT_SUPPORTED                336 */
    EIO,          /* ERROR_NOT_READ_FROM_COPY                     337 */
    -1,           /*                                              338 */
    -1,           /*                                              339 */
    -1,           /*                                              340 */
    -1,           /*                                              341 */
    -1,           /*                                              342 */
    -1,           /*                                              343 */
    -1,           /*                                              344 */
    -1,           /*                                              345 */
    -1,           /*                                              346 */
    -1,           /*                                              347 */
    -1,           /*                                              348 */
    -1,           /*                                              349 */
    EAGAIN,       /* ERROR_FAIL_NOACTION_REBOOT                   350 */
    EBUSY,        /* ERROR_FAIL_SHUTDOWN                          351 */
    EBUSY,        /* ERROR_FAIL_RESTART                           352 */
    EMFILE,       /* ERROR_MAX_SESSIONS_REACHED                   353 */
    -1,           /*                                              354 */
    -1,           /*                                              355 */
    -1,           /*                                              356 */
    -1,           /*                                              357 */
    -1,           /*                                              358 */
    -1,           /*                                              359 */
    -1,           /*                                              360 */
    -1,           /*                                              361 */
    -1,           /*                                              362 */
    -1,           /*                                              363 */
    -1,           /*                                              364 */
    -1,           /*                                              365 */
    -1,           /*                                              366 */
    -1,           /*                                              367 */
    -1,           /*                                              368 */
    -1,           /*                                              369 */
    -1,           /*                                              370 */
    -1,           /*                                              371 */
    -1,           /*                                              372 */
    -1,           /*                                              373 */
    -1,           /*                                              374 */
    -1,           /*                                              375 */
    -1,           /*                                              376 */
    -1,           /*                                              377 */
    -1,           /*                                              378 */
    -1,           /*                                              379 */
    -1,           /*                                              380 */
    -1,           /*                                              381 */
    -1,           /*                                              382 */
    -1,           /*                                              383 */
    -1,           /*                                              384 */
    -1,           /*                                              385 */
    -1,           /*                                              386 */
    -1,           /*                                              387 */
    -1,           /*                                              388 */
    -1,           /*                                              389 */
    -1,           /*                                              390 */
    -1,           /*                                              391 */
    -1,           /*                                              392 */
    -1,           /*                                              393 */
    -1,           /*                                              394 */
    -1,           /*                                              395 */
    -1,           /*                                              396 */
    -1,           /*                                              397 */
    -1,           /*                                              398 */
    -1,           /*                                              399 */
    EALREADY,     /* ERROR_THREAD_MODE_ALREADY_BACKGROUND         400 */
    EPERM,        /* ERROR_THREAD_MODE_NOT_BACKGROUND             401 */
    EALREADY,     /* ERROR_PROCESS_MODE_ALREADY_BACKGROUND        402 */
    EPERM,        /* ERROR_PROCESS_MODE_NOT_BACKGROUND            403 */
    -1,           /*                                              404 */
    -1,           /*                                              405 */
    -1,           /*                                              406 */
    -1,           /*                                              407 */
    -1,           /*                                              408 */
    -1,           /*                                              409 */
    -1,           /*                                              410 */
    -1,           /*                                              411 */
    -1,           /*                                              412 */
    -1,           /*                                              413 */
    -1,           /*                                              414 */
    -1,           /*                                              415 */
    -1,           /*                                              416 */
    -1,           /*                                              417 */
    -1,           /*                                              418 */
    -1,           /*                                              419 */
    -1,           /*                                              420 */
    -1,           /*                                              421 */
    -1,           /*                                              422 */
    -1,           /*                                              423 */
    -1,           /*                                              424 */
    -1,           /*                                              425 */
    -1,           /*                                              426 */
    -1,           /*                                              427 */
    -1,           /*                                              428 */
    -1,           /*                                              429 */
    -1,           /*                                              430 */
    -1,           /*                                              431 */
    -1,           /*                                              432 */
    -1,           /*                                              433 */
    -1,           /*                                              434 */
    -1,           /*                                              435 */
    -1,           /*                                              436 */
    -1,           /*                                              437 */
    -1,           /*                                              438 */
    -1,           /*                                              439 */
    -1,           /*                                              440 */
    -1,           /*                                              441 */
    -1,           /*                                              442 */
    -1,           /*                                              443 */
    -1,           /*                                              444 */
    -1,           /*                                              445 */
    -1,           /*                                              446 */
    -1,           /*                                              447 */
    -1,           /*                                              448 */
    -1,           /*                                              449 */
    -1,           /*                                              450 */
    -1,           /*                                              451 */
    -1,           /*                                              452 */
    -1,           /*                                              453 */
    -1,           /*                                              454 */
    -1,           /*                                              455 */
    -1,           /*                                              456 */
    -1,           /*                                              457 */
    -1,           /*                                              458 */
    -1,           /*                                              459 */
    -1,           /*                                              460 */
    -1,           /*                                              461 */
    -1,           /*                                              462 */
    -1,           /*                                              463 */
    -1,           /*                                              464 */
    -1,           /*                                              465 */
    -1,           /*                                              466 */
    -1,           /*                                              467 */
    -1,           /*                                              468 */
    -1,           /*                                              469 */
    -1,           /*                                              470 */
    -1,           /*                                              471 */
    -1,           /*                                              472 */
    -1,           /*                                              473 */
    -1,           /*                                              474 */
    -1,           /*                                              475 */
    -1,           /*                                              476 */
    -1,           /*                                              477 */
    -1,           /*                                              478 */
    -1,           /*                                              479 */
    -1,           /*                                              480 */
    -1,           /*                                              481 */
    -1,           /*                                              482 */
    -1,           /*                                              483 */
    -1,           /*                                              484 */
    -1,           /*                                              485 */
    -1,           /*                                              486 */
    EINVAL,       /* ERROR_INVALID_ADDRESS                        487 */
    -1,           /*                                              488 */
    -1,           /*                                              489 */
    -1,           /*                                              490 */
    -1,           /*                                              491 */
    -1,           /*                                              492 */
    -1,           /*                                              493 */
    -1,           /*                                              494 */
    -1,           /*                                              495 */
    -1,           /*                                              496 */
    -1,           /*                                              497 */
    -1,           /*                                              498 */
    -1,           /*                                              499 */
    // TODO the rest, there are still important errors that needs to be translated.
};

int gp_errno_from_win32_error(uint32_t error)
{
    if (error >= gp_countof(gp_s_error_table))
        return -1;
    error = gp_s_error_table[error];
    if (error == (uint8_t)-1)
        return -1;
    return error;
}
#endif // GP_TARGET_OS_WINDOWS
