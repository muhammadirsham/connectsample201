/* Header file automatically generated from windows.system.profile.systemmanufacturers.idl */
/*
 * File built with Microsoft(R) MIDLRT Compiler Engine Version 10.00.0226 
 */

#pragma warning( disable: 4049 )  /* more than 64k source lines */

/* verify that the <rpcndr.h> version is high enough to compile this file*/
#ifndef __REQUIRED_RPCNDR_H_VERSION__
#define __REQUIRED_RPCNDR_H_VERSION__ 500
#endif

/* verify that the <rpcsal.h> version is high enough to compile this file*/
#ifndef __REQUIRED_RPCSAL_H_VERSION__
#define __REQUIRED_RPCSAL_H_VERSION__ 100
#endif

#include <rpc.h>
#include <rpcndr.h>

#ifndef __RPCNDR_H_VERSION__
#error this stub requires an updated version of <rpcndr.h>
#endif /* __RPCNDR_H_VERSION__ */

#ifndef COM_NO_WINDOWS_H
#include <windows.h>
#include <ole2.h>
#endif /*COM_NO_WINDOWS_H*/
#ifndef __windows2Esystem2Eprofile2Esystemmanufacturers_h__
#define __windows2Esystem2Eprofile2Esystemmanufacturers_h__
#ifndef __windows2Esystem2Eprofile2Esystemmanufacturers_p_h__
#define __windows2Esystem2Eprofile2Esystemmanufacturers_p_h__


#pragma once

//
// Deprecated attribute support
//

#pragma push_macro("DEPRECATED")
#undef DEPRECATED

#if !defined(DISABLE_WINRT_DEPRECATION)
#if defined(__cplusplus)
#if __cplusplus >= 201402
#define DEPRECATED(x) [[deprecated(x)]]
#define DEPRECATEDENUMERATOR(x) [[deprecated(x)]]
#elif defined(_MSC_VER)
#if _MSC_VER >= 1900
#define DEPRECATED(x) [[deprecated(x)]]
#define DEPRECATEDENUMERATOR(x) [[deprecated(x)]]
#else
#define DEPRECATED(x) __declspec(deprecated(x))
#define DEPRECATEDENUMERATOR(x)
#endif // _MSC_VER >= 1900
#else // Not Standard C++ or MSVC, ignore the construct.
#define DEPRECATED(x)
#define DEPRECATEDENUMERATOR(x)
#endif  // C++ deprecation
#else // C - disable deprecation
#define DEPRECATED(x)
#define DEPRECATEDENUMERATOR(x)
#endif
#else // Deprecation is disabled
#define DEPRECATED(x)
#define DEPRECATEDENUMERATOR(x)
#endif  /* DEPRECATED */

// Disable Deprecation for this header, MIDL verifies that cross-type access is acceptable
#ifdef __clang__
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wdeprecated-declarations"
#else
#pragma warning(push)
#pragma warning(disable: 4996)
#endif

// Ensure that the setting of the /ns_prefix command line switch is consistent for all headers.
// If you get an error from the compiler indicating "warning C4005: 'CHECK_NS_PREFIX_STATE': macro redefinition", this
// indicates that you have included two different headers with different settings for the /ns_prefix MIDL command line switch
#if !defined(DISABLE_NS_PREFIX_CHECKS)
#define CHECK_NS_PREFIX_STATE "always"
#endif // !defined(DISABLE_NS_PREFIX_CHECKS)


#pragma push_macro("MIDL_CONST_ID")
#undef MIDL_CONST_ID
#define MIDL_CONST_ID const __declspec(selectany)


//  API Contract Inclusion Definitions
#if !defined(SPECIFIC_API_CONTRACT_DEFINITIONS)
#if !defined(WINDOWS_APPLICATIONMODEL_CALLS_CALLSPHONECONTRACT_VERSION)
#define WINDOWS_APPLICATIONMODEL_CALLS_CALLSPHONECONTRACT_VERSION 0x50000
#endif // defined(WINDOWS_APPLICATIONMODEL_CALLS_CALLSPHONECONTRACT_VERSION)

#if !defined(WINDOWS_FOUNDATION_FOUNDATIONCONTRACT_VERSION)
#define WINDOWS_FOUNDATION_FOUNDATIONCONTRACT_VERSION 0x30000
#endif // defined(WINDOWS_FOUNDATION_FOUNDATIONCONTRACT_VERSION)

#if !defined(WINDOWS_FOUNDATION_UNIVERSALAPICONTRACT_VERSION)
#define WINDOWS_FOUNDATION_UNIVERSALAPICONTRACT_VERSION 0x80000
#endif // defined(WINDOWS_FOUNDATION_UNIVERSALAPICONTRACT_VERSION)

#if !defined(WINDOWS_NETWORKING_SOCKETS_CONTROLCHANNELTRIGGERCONTRACT_VERSION)
#define WINDOWS_NETWORKING_SOCKETS_CONTROLCHANNELTRIGGERCONTRACT_VERSION 0x30000
#endif // defined(WINDOWS_NETWORKING_SOCKETS_CONTROLCHANNELTRIGGERCONTRACT_VERSION)

#if !defined(WINDOWS_PHONE_PHONECONTRACT_VERSION)
#define WINDOWS_PHONE_PHONECONTRACT_VERSION 0x10000
#endif // defined(WINDOWS_PHONE_PHONECONTRACT_VERSION)

#if !defined(WINDOWS_PHONE_PHONEINTERNALCONTRACT_VERSION)
#define WINDOWS_PHONE_PHONEINTERNALCONTRACT_VERSION 0x10000
#endif // defined(WINDOWS_PHONE_PHONEINTERNALCONTRACT_VERSION)

#if !defined(WINDOWS_SYSTEM_PROFILE_SYSTEMMANUFACTURERS_SYSTEMMANUFACTURERSCONTRACT_VERSION)
#define WINDOWS_SYSTEM_PROFILE_SYSTEMMANUFACTURERS_SYSTEMMANUFACTURERSCONTRACT_VERSION 0x30000
#endif // defined(WINDOWS_SYSTEM_PROFILE_SYSTEMMANUFACTURERS_SYSTEMMANUFACTURERSCONTRACT_VERSION)

#if !defined(WINDOWS_UI_WEBUI_CORE_WEBUICOMMANDBARCONTRACT_VERSION)
#define WINDOWS_UI_WEBUI_CORE_WEBUICOMMANDBARCONTRACT_VERSION 0x10000
#endif // defined(WINDOWS_UI_WEBUI_CORE_WEBUICOMMANDBARCONTRACT_VERSION)

#endif // defined(SPECIFIC_API_CONTRACT_DEFINITIONS)


// Header files for imported files
#include "inspectable.h"
#include "AsyncInfo.h"
#include "EventToken.h"
#include "windowscontracts.h"
#include "Windows.Foundation.h"

#if defined(__cplusplus) && !defined(CINTERFACE)
/* Forward Declarations */
#ifndef ____x_ABI_CWindows_CSystem_CProfile_CSystemManufacturers_CIOemSupportInfo_FWD_DEFINED__
#define ____x_ABI_CWindows_CSystem_CProfile_CSystemManufacturers_CIOemSupportInfo_FWD_DEFINED__
namespace ABI {
    namespace Windows {
        namespace System {
            namespace Profile {
                namespace SystemManufacturers {
                    interface IOemSupportInfo;
                } /* SystemManufacturers */
            } /* Profile */
        } /* System */
    } /* Windows */} /* ABI */
#define __x_ABI_CWindows_CSystem_CProfile_CSystemManufacturers_CIOemSupportInfo ABI::Windows::System::Profile::SystemManufacturers::IOemSupportInfo

#endif // ____x_ABI_CWindows_CSystem_CProfile_CSystemManufacturers_CIOemSupportInfo_FWD_DEFINED__

#ifndef ____x_ABI_CWindows_CSystem_CProfile_CSystemManufacturers_CISmbiosInformationStatics_FWD_DEFINED__
#define ____x_ABI_CWindows_CSystem_CProfile_CSystemManufacturers_CISmbiosInformationStatics_FWD_DEFINED__
namespace ABI {
    namespace Windows {
        namespace System {
            namespace Profile {
                namespace SystemManufacturers {
                    interface ISmbiosInformationStatics;
                } /* SystemManufacturers */
            } /* Profile */
        } /* System */
    } /* Windows */} /* ABI */
#define __x_ABI_CWindows_CSystem_CProfile_CSystemManufacturers_CISmbiosInformationStatics ABI::Windows::System::Profile::SystemManufacturers::ISmbiosInformationStatics

#endif // ____x_ABI_CWindows_CSystem_CProfile_CSystemManufacturers_CISmbiosInformationStatics_FWD_DEFINED__

#ifndef ____x_ABI_CWindows_CSystem_CProfile_CSystemManufacturers_CISystemSupportDeviceInfo_FWD_DEFINED__
#define ____x_ABI_CWindows_CSystem_CProfile_CSystemManufacturers_CISystemSupportDeviceInfo_FWD_DEFINED__
namespace ABI {
    namespace Windows {
        namespace System {
            namespace Profile {
                namespace SystemManufacturers {
                    interface ISystemSupportDeviceInfo;
                } /* SystemManufacturers */
            } /* Profile */
        } /* System */
    } /* Windows */} /* ABI */
#define __x_ABI_CWindows_CSystem_CProfile_CSystemManufacturers_CISystemSupportDeviceInfo ABI::Windows::System::Profile::SystemManufacturers::ISystemSupportDeviceInfo

#endif // ____x_ABI_CWindows_CSystem_CProfile_CSystemManufacturers_CISystemSupportDeviceInfo_FWD_DEFINED__

#ifndef ____x_ABI_CWindows_CSystem_CProfile_CSystemManufacturers_CISystemSupportInfoStatics_FWD_DEFINED__
#define ____x_ABI_CWindows_CSystem_CProfile_CSystemManufacturers_CISystemSupportInfoStatics_FWD_DEFINED__
namespace ABI {
    namespace Windows {
        namespace System {
            namespace Profile {
                namespace SystemManufacturers {
                    interface ISystemSupportInfoStatics;
                } /* SystemManufacturers */
            } /* Profile */
        } /* System */
    } /* Windows */} /* ABI */
#define __x_ABI_CWindows_CSystem_CProfile_CSystemManufacturers_CISystemSupportInfoStatics ABI::Windows::System::Profile::SystemManufacturers::ISystemSupportInfoStatics

#endif // ____x_ABI_CWindows_CSystem_CProfile_CSystemManufacturers_CISystemSupportInfoStatics_FWD_DEFINED__

#ifndef ____x_ABI_CWindows_CSystem_CProfile_CSystemManufacturers_CISystemSupportInfoStatics2_FWD_DEFINED__
#define ____x_ABI_CWindows_CSystem_CProfile_CSystemManufacturers_CISystemSupportInfoStatics2_FWD_DEFINED__
namespace ABI {
    namespace Windows {
        namespace System {
            namespace Profile {
                namespace SystemManufacturers {
                    interface ISystemSupportInfoStatics2;
                } /* SystemManufacturers */
            } /* Profile */
        } /* System */
    } /* Windows */} /* ABI */
#define __x_ABI_CWindows_CSystem_CProfile_CSystemManufacturers_CISystemSupportInfoStatics2 ABI::Windows::System::Profile::SystemManufacturers::ISystemSupportInfoStatics2

#endif // ____x_ABI_CWindows_CSystem_CProfile_CSystemManufacturers_CISystemSupportInfoStatics2_FWD_DEFINED__


namespace ABI {
    namespace Windows {
        namespace Foundation {
            class Uri;
        } /* Foundation */
    } /* Windows */} /* ABI */

#ifndef ____x_ABI_CWindows_CFoundation_CIUriRuntimeClass_FWD_DEFINED__
#define ____x_ABI_CWindows_CFoundation_CIUriRuntimeClass_FWD_DEFINED__
namespace ABI {
    namespace Windows {
        namespace Foundation {
            interface IUriRuntimeClass;
        } /* Foundation */
    } /* Windows */} /* ABI */
#define __x_ABI_CWindows_CFoundation_CIUriRuntimeClass ABI::Windows::Foundation::IUriRuntimeClass

#endif // ____x_ABI_CWindows_CFoundation_CIUriRuntimeClass_FWD_DEFINED__










namespace ABI {
    namespace Windows {
        namespace System {
            namespace Profile {
                namespace SystemManufacturers {
                    class OemSupportInfo;
                } /* SystemManufacturers */
            } /* Profile */
        } /* System */
    } /* Windows */} /* ABI */



namespace ABI {
    namespace Windows {
        namespace System {
            namespace Profile {
                namespace SystemManufacturers {
                    class SystemSupportDeviceInfo;
                } /* SystemManufacturers */
            } /* Profile */
        } /* System */
    } /* Windows */} /* ABI */









/*
 *
 * Interface Windows.System.Profile.SystemManufacturers.IOemSupportInfo
 *
 * Introduced to Windows.System.Profile.SystemManufacturers.SystemManufacturersContract in version 2.0
 *
 *
 * Interface is a part of the implementation of type Windows.System.Profile.SystemManufacturers.OemSupportInfo
 *
 *
 */
#if WINDOWS_SYSTEM_PROFILE_SYSTEMMANUFACTURERS_SYSTEMMANUFACTURERSCONTRACT_VERSION >= 0x20000
#if !defined(____x_ABI_CWindows_CSystem_CProfile_CSystemManufacturers_CIOemSupportInfo_INTERFACE_DEFINED__)
#define ____x_ABI_CWindows_CSystem_CProfile_CSystemManufacturers_CIOemSupportInfo_INTERFACE_DEFINED__
extern const __declspec(selectany) _Null_terminated_ WCHAR InterfaceName_Windows_System_Profile_SystemManufacturers_IOemSupportInfo[] = L"Windows.System.Profile.SystemManufacturers.IOemSupportInfo";
namespace ABI {
    namespace Windows {
        namespace System {
            namespace Profile {
                namespace SystemManufacturers {
                    /* [object, uuid("8D2EAE55-87EF-4266-86D0-C4AFBEB29BB9"), exclusiveto, contract] */
                    MIDL_INTERFACE("8D2EAE55-87EF-4266-86D0-C4AFBEB29BB9")
                    IOemSupportInfo : public IInspectable
                    {
                    public:
                        /* [propget] */virtual HRESULT STDMETHODCALLTYPE get_SupportLink(
                            /* [retval, out] */__RPC__deref_out_opt ABI::Windows::Foundation::IUriRuntimeClass * * value
                            ) = 0;
                        /* [propget] */virtual HRESULT STDMETHODCALLTYPE get_SupportAppLink(
                            /* [retval, out] */__RPC__deref_out_opt ABI::Windows::Foundation::IUriRuntimeClass * * value
                            ) = 0;
                        /* [propget] */virtual HRESULT STDMETHODCALLTYPE get_SupportProvider(
                            /* [retval, out] */__RPC__deref_out_opt HSTRING * value
                            ) = 0;
                        
                    };

                    extern MIDL_CONST_ID IID & IID_IOemSupportInfo=_uuidof(IOemSupportInfo);
                    
                } /* SystemManufacturers */
            } /* Profile */
        } /* System */
    } /* Windows */} /* ABI */

EXTERN_C const IID IID___x_ABI_CWindows_CSystem_CProfile_CSystemManufacturers_CIOemSupportInfo;
#endif /* !defined(____x_ABI_CWindows_CSystem_CProfile_CSystemManufacturers_CIOemSupportInfo_INTERFACE_DEFINED__) */
#endif // WINDOWS_SYSTEM_PROFILE_SYSTEMMANUFACTURERS_SYSTEMMANUFACTURERSCONTRACT_VERSION >= 0x20000


/*
 *
 * Interface Windows.System.Profile.SystemManufacturers.ISmbiosInformationStatics
 *
 * Introduced to Windows.System.Profile.SystemManufacturers.SystemManufacturersContract in version 1.0
 *
 *
 * Interface is a part of the implementation of type Windows.System.Profile.SystemManufacturers.SmbiosInformation
 *
 *
 */
#if WINDOWS_SYSTEM_PROFILE_SYSTEMMANUFACTURERS_SYSTEMMANUFACTURERSCONTRACT_VERSION >= 0x10000
#if !defined(____x_ABI_CWindows_CSystem_CProfile_CSystemManufacturers_CISmbiosInformationStatics_INTERFACE_DEFINED__)
#define ____x_ABI_CWindows_CSystem_CProfile_CSystemManufacturers_CISmbiosInformationStatics_INTERFACE_DEFINED__
extern const __declspec(selectany) _Null_terminated_ WCHAR InterfaceName_Windows_System_Profile_SystemManufacturers_ISmbiosInformationStatics[] = L"Windows.System.Profile.SystemManufacturers.ISmbiosInformationStatics";
namespace ABI {
    namespace Windows {
        namespace System {
            namespace Profile {
                namespace SystemManufacturers {
                    /* [object, uuid("080CCA7C-637C-48C4-B728-F9273812DB8E"), exclusiveto, contract] */
                    MIDL_INTERFACE("080CCA7C-637C-48C4-B728-F9273812DB8E")
                    ISmbiosInformationStatics : public IInspectable
                    {
                    public:
                        /* [propget] */virtual HRESULT STDMETHODCALLTYPE get_SerialNumber(
                            /* [retval, out] */__RPC__deref_out_opt HSTRING * value
                            ) = 0;
                        
                    };

                    extern MIDL_CONST_ID IID & IID_ISmbiosInformationStatics=_uuidof(ISmbiosInformationStatics);
                    
                } /* SystemManufacturers */
            } /* Profile */
        } /* System */
    } /* Windows */} /* ABI */

EXTERN_C const IID IID___x_ABI_CWindows_CSystem_CProfile_CSystemManufacturers_CISmbiosInformationStatics;
#endif /* !defined(____x_ABI_CWindows_CSystem_CProfile_CSystemManufacturers_CISmbiosInformationStatics_INTERFACE_DEFINED__) */
#endif // WINDOWS_SYSTEM_PROFILE_SYSTEMMANUFACTURERS_SYSTEMMANUFACTURERSCONTRACT_VERSION >= 0x10000


/*
 *
 * Interface Windows.System.Profile.SystemManufacturers.ISystemSupportDeviceInfo
 *
 * Introduced to Windows.System.Profile.SystemManufacturers.SystemManufacturersContract in version 3.0
 *
 *
 * Interface is a part of the implementation of type Windows.System.Profile.SystemManufacturers.SystemSupportDeviceInfo
 *
 *
 */
#if WINDOWS_SYSTEM_PROFILE_SYSTEMMANUFACTURERS_SYSTEMMANUFACTURERSCONTRACT_VERSION >= 0x30000
#if !defined(____x_ABI_CWindows_CSystem_CProfile_CSystemManufacturers_CISystemSupportDeviceInfo_INTERFACE_DEFINED__)
#define ____x_ABI_CWindows_CSystem_CProfile_CSystemManufacturers_CISystemSupportDeviceInfo_INTERFACE_DEFINED__
extern const __declspec(selectany) _Null_terminated_ WCHAR InterfaceName_Windows_System_Profile_SystemManufacturers_ISystemSupportDeviceInfo[] = L"Windows.System.Profile.SystemManufacturers.ISystemSupportDeviceInfo";
namespace ABI {
    namespace Windows {
        namespace System {
            namespace Profile {
                namespace SystemManufacturers {
                    /* [object, uuid("05880B99-8247-441B-A996-A1784BAB79A8"), exclusiveto, contract] */
                    MIDL_INTERFACE("05880B99-8247-441B-A996-A1784BAB79A8")
                    ISystemSupportDeviceInfo : public IInspectable
                    {
                    public:
                        /* [propget] */virtual HRESULT STDMETHODCALLTYPE get_OperatingSystem(
                            /* [retval, out] */__RPC__deref_out_opt HSTRING * value
                            ) = 0;
                        /* [propget] */virtual HRESULT STDMETHODCALLTYPE get_FriendlyName(
                            /* [retval, out] */__RPC__deref_out_opt HSTRING * value
                            ) = 0;
                        /* [propget] */virtual HRESULT STDMETHODCALLTYPE get_SystemManufacturer(
                            /* [retval, out] */__RPC__deref_out_opt HSTRING * value
                            ) = 0;
                        /* [propget] */virtual HRESULT STDMETHODCALLTYPE get_SystemProductName(
                            /* [retval, out] */__RPC__deref_out_opt HSTRING * value
                            ) = 0;
                        /* [propget] */virtual HRESULT STDMETHODCALLTYPE get_SystemSku(
                            /* [retval, out] */__RPC__deref_out_opt HSTRING * value
                            ) = 0;
                        /* [propget] */virtual HRESULT STDMETHODCALLTYPE get_SystemHardwareVersion(
                            /* [retval, out] */__RPC__deref_out_opt HSTRING * value
                            ) = 0;
                        /* [propget] */virtual HRESULT STDMETHODCALLTYPE get_SystemFirmwareVersion(
                            /* [retval, out] */__RPC__deref_out_opt HSTRING * value
                            ) = 0;
                        
                    };

                    extern MIDL_CONST_ID IID & IID_ISystemSupportDeviceInfo=_uuidof(ISystemSupportDeviceInfo);
                    
                } /* SystemManufacturers */
            } /* Profile */
        } /* System */
    } /* Windows */} /* ABI */

EXTERN_C const IID IID___x_ABI_CWindows_CSystem_CProfile_CSystemManufacturers_CISystemSupportDeviceInfo;
#endif /* !defined(____x_ABI_CWindows_CSystem_CProfile_CSystemManufacturers_CISystemSupportDeviceInfo_INTERFACE_DEFINED__) */
#endif // WINDOWS_SYSTEM_PROFILE_SYSTEMMANUFACTURERS_SYSTEMMANUFACTURERSCONTRACT_VERSION >= 0x30000


/*
 *
 * Interface Windows.System.Profile.SystemManufacturers.ISystemSupportInfoStatics
 *
 * Introduced to Windows.System.Profile.SystemManufacturers.SystemManufacturersContract in version 2.0
 *
 *
 * Interface is a part of the implementation of type Windows.System.Profile.SystemManufacturers.SystemSupportInfo
 *
 *
 */
#if WINDOWS_SYSTEM_PROFILE_SYSTEMMANUFACTURERS_SYSTEMMANUFACTURERSCONTRACT_VERSION >= 0x20000
#if !defined(____x_ABI_CWindows_CSystem_CProfile_CSystemManufacturers_CISystemSupportInfoStatics_INTERFACE_DEFINED__)
#define ____x_ABI_CWindows_CSystem_CProfile_CSystemManufacturers_CISystemSupportInfoStatics_INTERFACE_DEFINED__
extern const __declspec(selectany) _Null_terminated_ WCHAR InterfaceName_Windows_System_Profile_SystemManufacturers_ISystemSupportInfoStatics[] = L"Windows.System.Profile.SystemManufacturers.ISystemSupportInfoStatics";
namespace ABI {
    namespace Windows {
        namespace System {
            namespace Profile {
                namespace SystemManufacturers {
                    /* [object, uuid("EF750974-C422-45D7-A44D-5C1C0043A2B3"), exclusiveto, contract] */
                    MIDL_INTERFACE("EF750974-C422-45D7-A44D-5C1C0043A2B3")
                    ISystemSupportInfoStatics : public IInspectable
                    {
                    public:
                        /* [propget] */virtual HRESULT STDMETHODCALLTYPE get_LocalSystemEdition(
                            /* [retval, out] */__RPC__deref_out_opt HSTRING * value
                            ) = 0;
                        /* [propget] */virtual HRESULT STDMETHODCALLTYPE get_OemSupportInfo(
                            /* [retval, out] */__RPC__deref_out_opt ABI::Windows::System::Profile::SystemManufacturers::IOemSupportInfo * * value
                            ) = 0;
                        
                    };

                    extern MIDL_CONST_ID IID & IID_ISystemSupportInfoStatics=_uuidof(ISystemSupportInfoStatics);
                    
                } /* SystemManufacturers */
            } /* Profile */
        } /* System */
    } /* Windows */} /* ABI */

EXTERN_C const IID IID___x_ABI_CWindows_CSystem_CProfile_CSystemManufacturers_CISystemSupportInfoStatics;
#endif /* !defined(____x_ABI_CWindows_CSystem_CProfile_CSystemManufacturers_CISystemSupportInfoStatics_INTERFACE_DEFINED__) */
#endif // WINDOWS_SYSTEM_PROFILE_SYSTEMMANUFACTURERS_SYSTEMMANUFACTURERSCONTRACT_VERSION >= 0x20000


/*
 *
 * Interface Windows.System.Profile.SystemManufacturers.ISystemSupportInfoStatics2
 *
 * Introduced to Windows.System.Profile.SystemManufacturers.SystemManufacturersContract in version 3.0
 *
 *
 * Interface is a part of the implementation of type Windows.System.Profile.SystemManufacturers.SystemSupportInfo
 *
 *
 */
#if WINDOWS_SYSTEM_PROFILE_SYSTEMMANUFACTURERS_SYSTEMMANUFACTURERSCONTRACT_VERSION >= 0x30000
#if !defined(____x_ABI_CWindows_CSystem_CProfile_CSystemManufacturers_CISystemSupportInfoStatics2_INTERFACE_DEFINED__)
#define ____x_ABI_CWindows_CSystem_CProfile_CSystemManufacturers_CISystemSupportInfoStatics2_INTERFACE_DEFINED__
extern const __declspec(selectany) _Null_terminated_ WCHAR InterfaceName_Windows_System_Profile_SystemManufacturers_ISystemSupportInfoStatics2[] = L"Windows.System.Profile.SystemManufacturers.ISystemSupportInfoStatics2";
namespace ABI {
    namespace Windows {
        namespace System {
            namespace Profile {
                namespace SystemManufacturers {
                    /* [object, uuid("33F349A4-3FA1-4986-AA4B-057420455E6D"), exclusiveto, contract] */
                    MIDL_INTERFACE("33F349A4-3FA1-4986-AA4B-057420455E6D")
                    ISystemSupportInfoStatics2 : public IInspectable
                    {
                    public:
                        /* [propget] */virtual HRESULT STDMETHODCALLTYPE get_LocalDeviceInfo(
                            /* [retval, out] */__RPC__deref_out_opt ABI::Windows::System::Profile::SystemManufacturers::ISystemSupportDeviceInfo * * value
                            ) = 0;
                        
                    };

                    extern MIDL_CONST_ID IID & IID_ISystemSupportInfoStatics2=_uuidof(ISystemSupportInfoStatics2);
                    
                } /* SystemManufacturers */
            } /* Profile */
        } /* System */
    } /* Windows */} /* ABI */

EXTERN_C const IID IID___x_ABI_CWindows_CSystem_CProfile_CSystemManufacturers_CISystemSupportInfoStatics2;
#endif /* !defined(____x_ABI_CWindows_CSystem_CProfile_CSystemManufacturers_CISystemSupportInfoStatics2_INTERFACE_DEFINED__) */
#endif // WINDOWS_SYSTEM_PROFILE_SYSTEMMANUFACTURERS_SYSTEMMANUFACTURERSCONTRACT_VERSION >= 0x30000


/*
 *
 * Class Windows.System.Profile.SystemManufacturers.OemSupportInfo
 *
 * Introduced to Windows.System.Profile.SystemManufacturers.SystemManufacturersContract in version 2.0
 *
 *
 * Class implements the following interfaces:
 *    Windows.System.Profile.SystemManufacturers.IOemSupportInfo ** Default Interface **
 *
 * Class Threading Model:  Both Single and Multi Threaded Apartment
 *
 * Class Marshaling Behavior:  Agile - Class is agile
 *
 */
#if WINDOWS_SYSTEM_PROFILE_SYSTEMMANUFACTURERS_SYSTEMMANUFACTURERSCONTRACT_VERSION >= 0x20000

#ifndef RUNTIMECLASS_Windows_System_Profile_SystemManufacturers_OemSupportInfo_DEFINED
#define RUNTIMECLASS_Windows_System_Profile_SystemManufacturers_OemSupportInfo_DEFINED
extern const __declspec(selectany) _Null_terminated_ WCHAR RuntimeClass_Windows_System_Profile_SystemManufacturers_OemSupportInfo[] = L"Windows.System.Profile.SystemManufacturers.OemSupportInfo";
#endif
#endif // WINDOWS_SYSTEM_PROFILE_SYSTEMMANUFACTURERS_SYSTEMMANUFACTURERSCONTRACT_VERSION >= 0x20000


/*
 *
 * Class Windows.System.Profile.SystemManufacturers.SmbiosInformation
 *
 * Introduced to Windows.System.Profile.SystemManufacturers.SystemManufacturersContract in version 1.0
 *
 *
 * RuntimeClass contains static methods.
 *   Static Methods exist on the Windows.System.Profile.SystemManufacturers.ISmbiosInformationStatics interface starting with version 1.0 of the Windows.System.Profile.SystemManufacturers.SystemManufacturersContract API contract
 *
 * Class Marshaling Behavior:  Agile - Class is agile
 *
 */
#if WINDOWS_SYSTEM_PROFILE_SYSTEMMANUFACTURERS_SYSTEMMANUFACTURERSCONTRACT_VERSION >= 0x10000
#ifndef RUNTIMECLASS_Windows_System_Profile_SystemManufacturers_SmbiosInformation_DEFINED
#define RUNTIMECLASS_Windows_System_Profile_SystemManufacturers_SmbiosInformation_DEFINED
extern const __declspec(selectany) _Null_terminated_ WCHAR RuntimeClass_Windows_System_Profile_SystemManufacturers_SmbiosInformation[] = L"Windows.System.Profile.SystemManufacturers.SmbiosInformation";
#endif
#endif // WINDOWS_SYSTEM_PROFILE_SYSTEMMANUFACTURERS_SYSTEMMANUFACTURERSCONTRACT_VERSION >= 0x10000


/*
 *
 * Class Windows.System.Profile.SystemManufacturers.SystemSupportDeviceInfo
 *
 * Introduced to Windows.System.Profile.SystemManufacturers.SystemManufacturersContract in version 3.0
 *
 *
 * Class implements the following interfaces:
 *    Windows.System.Profile.SystemManufacturers.ISystemSupportDeviceInfo ** Default Interface **
 *
 * Class Threading Model:  Both Single and Multi Threaded Apartment
 *
 * Class Marshaling Behavior:  Agile - Class is agile
 *
 */
#if WINDOWS_SYSTEM_PROFILE_SYSTEMMANUFACTURERS_SYSTEMMANUFACTURERSCONTRACT_VERSION >= 0x30000

#ifndef RUNTIMECLASS_Windows_System_Profile_SystemManufacturers_SystemSupportDeviceInfo_DEFINED
#define RUNTIMECLASS_Windows_System_Profile_SystemManufacturers_SystemSupportDeviceInfo_DEFINED
extern const __declspec(selectany) _Null_terminated_ WCHAR RuntimeClass_Windows_System_Profile_SystemManufacturers_SystemSupportDeviceInfo[] = L"Windows.System.Profile.SystemManufacturers.SystemSupportDeviceInfo";
#endif
#endif // WINDOWS_SYSTEM_PROFILE_SYSTEMMANUFACTURERS_SYSTEMMANUFACTURERSCONTRACT_VERSION >= 0x30000


/*
 *
 * Class Windows.System.Profile.SystemManufacturers.SystemSupportInfo
 *
 * Introduced to Windows.System.Profile.SystemManufacturers.SystemManufacturersContract in version 2.0
 *
 *
 * RuntimeClass contains static methods.
 *   Static Methods exist on the Windows.System.Profile.SystemManufacturers.ISystemSupportInfoStatics2 interface starting with version 3.0 of the Windows.System.Profile.SystemManufacturers.SystemManufacturersContract API contract
 *   Static Methods exist on the Windows.System.Profile.SystemManufacturers.ISystemSupportInfoStatics interface starting with version 2.0 of the Windows.System.Profile.SystemManufacturers.SystemManufacturersContract API contract
 *
 * Class Threading Model:  Both Single and Multi Threaded Apartment
 *
 * Class Marshaling Behavior:  Agile - Class is agile
 *
 */
#if WINDOWS_SYSTEM_PROFILE_SYSTEMMANUFACTURERS_SYSTEMMANUFACTURERSCONTRACT_VERSION >= 0x20000
#ifndef RUNTIMECLASS_Windows_System_Profile_SystemManufacturers_SystemSupportInfo_DEFINED
#define RUNTIMECLASS_Windows_System_Profile_SystemManufacturers_SystemSupportInfo_DEFINED
extern const __declspec(selectany) _Null_terminated_ WCHAR RuntimeClass_Windows_System_Profile_SystemManufacturers_SystemSupportInfo[] = L"Windows.System.Profile.SystemManufacturers.SystemSupportInfo";
#endif
#endif // WINDOWS_SYSTEM_PROFILE_SYSTEMMANUFACTURERS_SYSTEMMANUFACTURERSCONTRACT_VERSION >= 0x20000





#else // !defined(__cplusplus)
/* Forward Declarations */
#ifndef ____x_ABI_CWindows_CSystem_CProfile_CSystemManufacturers_CIOemSupportInfo_FWD_DEFINED__
#define ____x_ABI_CWindows_CSystem_CProfile_CSystemManufacturers_CIOemSupportInfo_FWD_DEFINED__
typedef interface __x_ABI_CWindows_CSystem_CProfile_CSystemManufacturers_CIOemSupportInfo __x_ABI_CWindows_CSystem_CProfile_CSystemManufacturers_CIOemSupportInfo;

#endif // ____x_ABI_CWindows_CSystem_CProfile_CSystemManufacturers_CIOemSupportInfo_FWD_DEFINED__

#ifndef ____x_ABI_CWindows_CSystem_CProfile_CSystemManufacturers_CISmbiosInformationStatics_FWD_DEFINED__
#define ____x_ABI_CWindows_CSystem_CProfile_CSystemManufacturers_CISmbiosInformationStatics_FWD_DEFINED__
typedef interface __x_ABI_CWindows_CSystem_CProfile_CSystemManufacturers_CISmbiosInformationStatics __x_ABI_CWindows_CSystem_CProfile_CSystemManufacturers_CISmbiosInformationStatics;

#endif // ____x_ABI_CWindows_CSystem_CProfile_CSystemManufacturers_CISmbiosInformationStatics_FWD_DEFINED__

#ifndef ____x_ABI_CWindows_CSystem_CProfile_CSystemManufacturers_CISystemSupportDeviceInfo_FWD_DEFINED__
#define ____x_ABI_CWindows_CSystem_CProfile_CSystemManufacturers_CISystemSupportDeviceInfo_FWD_DEFINED__
typedef interface __x_ABI_CWindows_CSystem_CProfile_CSystemManufacturers_CISystemSupportDeviceInfo __x_ABI_CWindows_CSystem_CProfile_CSystemManufacturers_CISystemSupportDeviceInfo;

#endif // ____x_ABI_CWindows_CSystem_CProfile_CSystemManufacturers_CISystemSupportDeviceInfo_FWD_DEFINED__

#ifndef ____x_ABI_CWindows_CSystem_CProfile_CSystemManufacturers_CISystemSupportInfoStatics_FWD_DEFINED__
#define ____x_ABI_CWindows_CSystem_CProfile_CSystemManufacturers_CISystemSupportInfoStatics_FWD_DEFINED__
typedef interface __x_ABI_CWindows_CSystem_CProfile_CSystemManufacturers_CISystemSupportInfoStatics __x_ABI_CWindows_CSystem_CProfile_CSystemManufacturers_CISystemSupportInfoStatics;

#endif // ____x_ABI_CWindows_CSystem_CProfile_CSystemManufacturers_CISystemSupportInfoStatics_FWD_DEFINED__

#ifndef ____x_ABI_CWindows_CSystem_CProfile_CSystemManufacturers_CISystemSupportInfoStatics2_FWD_DEFINED__
#define ____x_ABI_CWindows_CSystem_CProfile_CSystemManufacturers_CISystemSupportInfoStatics2_FWD_DEFINED__
typedef interface __x_ABI_CWindows_CSystem_CProfile_CSystemManufacturers_CISystemSupportInfoStatics2 __x_ABI_CWindows_CSystem_CProfile_CSystemManufacturers_CISystemSupportInfoStatics2;

#endif // ____x_ABI_CWindows_CSystem_CProfile_CSystemManufacturers_CISystemSupportInfoStatics2_FWD_DEFINED__

#ifndef ____x_ABI_CWindows_CFoundation_CIUriRuntimeClass_FWD_DEFINED__
#define ____x_ABI_CWindows_CFoundation_CIUriRuntimeClass_FWD_DEFINED__
typedef interface __x_ABI_CWindows_CFoundation_CIUriRuntimeClass __x_ABI_CWindows_CFoundation_CIUriRuntimeClass;

#endif // ____x_ABI_CWindows_CFoundation_CIUriRuntimeClass_FWD_DEFINED__




















/*
 *
 * Interface Windows.System.Profile.SystemManufacturers.IOemSupportInfo
 *
 * Introduced to Windows.System.Profile.SystemManufacturers.SystemManufacturersContract in version 2.0
 *
 *
 * Interface is a part of the implementation of type Windows.System.Profile.SystemManufacturers.OemSupportInfo
 *
 *
 */
#if WINDOWS_SYSTEM_PROFILE_SYSTEMMANUFACTURERS_SYSTEMMANUFACTURERSCONTRACT_VERSION >= 0x20000
#if !defined(____x_ABI_CWindows_CSystem_CProfile_CSystemManufacturers_CIOemSupportInfo_INTERFACE_DEFINED__)
#define ____x_ABI_CWindows_CSystem_CProfile_CSystemManufacturers_CIOemSupportInfo_INTERFACE_DEFINED__
extern const __declspec(selectany) _Null_terminated_ WCHAR InterfaceName_Windows_System_Profile_SystemManufacturers_IOemSupportInfo[] = L"Windows.System.Profile.SystemManufacturers.IOemSupportInfo";
/* [object, uuid("8D2EAE55-87EF-4266-86D0-C4AFBEB29BB9"), exclusiveto, contract] */
typedef struct __x_ABI_CWindows_CSystem_CProfile_CSystemManufacturers_CIOemSupportInfoVtbl
{
    BEGIN_INTERFACE
    HRESULT ( STDMETHODCALLTYPE *QueryInterface)(
    __RPC__in __x_ABI_CWindows_CSystem_CProfile_CSystemManufacturers_CIOemSupportInfo * This,
    /* [in] */ __RPC__in REFIID riid,
    /* [annotation][iid_is][out] */
    _COM_Outptr_  void **ppvObject
    );

ULONG ( STDMETHODCALLTYPE *AddRef )(
    __RPC__in __x_ABI_CWindows_CSystem_CProfile_CSystemManufacturers_CIOemSupportInfo * This
    );

ULONG ( STDMETHODCALLTYPE *Release )(
    __RPC__in __x_ABI_CWindows_CSystem_CProfile_CSystemManufacturers_CIOemSupportInfo * This
    );

HRESULT ( STDMETHODCALLTYPE *GetIids )(
    __RPC__in __x_ABI_CWindows_CSystem_CProfile_CSystemManufacturers_CIOemSupportInfo * This,
    /* [out] */ __RPC__out ULONG *iidCount,
    /* [size_is][size_is][out] */ __RPC__deref_out_ecount_full_opt(*iidCount) IID **iids
    );

HRESULT ( STDMETHODCALLTYPE *GetRuntimeClassName )(
    __RPC__in __x_ABI_CWindows_CSystem_CProfile_CSystemManufacturers_CIOemSupportInfo * This,
    /* [out] */ __RPC__deref_out_opt HSTRING *className
    );

HRESULT ( STDMETHODCALLTYPE *GetTrustLevel )(
    __RPC__in __x_ABI_CWindows_CSystem_CProfile_CSystemManufacturers_CIOemSupportInfo * This,
    /* [OUT ] */ __RPC__out TrustLevel *trustLevel
    );
/* [propget] */HRESULT ( STDMETHODCALLTYPE *get_SupportLink )(
        __x_ABI_CWindows_CSystem_CProfile_CSystemManufacturers_CIOemSupportInfo * This,
        /* [retval, out] */__RPC__deref_out_opt __x_ABI_CWindows_CFoundation_CIUriRuntimeClass * * value
        );
    /* [propget] */HRESULT ( STDMETHODCALLTYPE *get_SupportAppLink )(
        __x_ABI_CWindows_CSystem_CProfile_CSystemManufacturers_CIOemSupportInfo * This,
        /* [retval, out] */__RPC__deref_out_opt __x_ABI_CWindows_CFoundation_CIUriRuntimeClass * * value
        );
    /* [propget] */HRESULT ( STDMETHODCALLTYPE *get_SupportProvider )(
        __x_ABI_CWindows_CSystem_CProfile_CSystemManufacturers_CIOemSupportInfo * This,
        /* [retval, out] */__RPC__deref_out_opt HSTRING * value
        );
    END_INTERFACE
    
} __x_ABI_CWindows_CSystem_CProfile_CSystemManufacturers_CIOemSupportInfoVtbl;

interface __x_ABI_CWindows_CSystem_CProfile_CSystemManufacturers_CIOemSupportInfo
{
    CONST_VTBL struct __x_ABI_CWindows_CSystem_CProfile_CSystemManufacturers_CIOemSupportInfoVtbl *lpVtbl;
};

#ifdef COBJMACROS
#define __x_ABI_CWindows_CSystem_CProfile_CSystemManufacturers_CIOemSupportInfo_QueryInterface(This,riid,ppvObject) \
( (This)->lpVtbl->QueryInterface(This,riid,ppvObject) )

#define __x_ABI_CWindows_CSystem_CProfile_CSystemManufacturers_CIOemSupportInfo_AddRef(This) \
        ( (This)->lpVtbl->AddRef(This) )

#define __x_ABI_CWindows_CSystem_CProfile_CSystemManufacturers_CIOemSupportInfo_Release(This) \
        ( (This)->lpVtbl->Release(This) )

#define __x_ABI_CWindows_CSystem_CProfile_CSystemManufacturers_CIOemSupportInfo_GetIids(This,iidCount,iids) \
        ( (This)->lpVtbl->GetIids(This,iidCount,iids) )

#define __x_ABI_CWindows_CSystem_CProfile_CSystemManufacturers_CIOemSupportInfo_GetRuntimeClassName(This,className) \
        ( (This)->lpVtbl->GetRuntimeClassName(This,className) )

#define __x_ABI_CWindows_CSystem_CProfile_CSystemManufacturers_CIOemSupportInfo_GetTrustLevel(This,trustLevel) \
        ( (This)->lpVtbl->GetTrustLevel(This,trustLevel) )

#define __x_ABI_CWindows_CSystem_CProfile_CSystemManufacturers_CIOemSupportInfo_get_SupportLink(This,value) \
    ( (This)->lpVtbl->get_SupportLink(This,value) )

#define __x_ABI_CWindows_CSystem_CProfile_CSystemManufacturers_CIOemSupportInfo_get_SupportAppLink(This,value) \
    ( (This)->lpVtbl->get_SupportAppLink(This,value) )

#define __x_ABI_CWindows_CSystem_CProfile_CSystemManufacturers_CIOemSupportInfo_get_SupportProvider(This,value) \
    ( (This)->lpVtbl->get_SupportProvider(This,value) )


#endif /* COBJMACROS */


EXTERN_C const IID IID___x_ABI_CWindows_CSystem_CProfile_CSystemManufacturers_CIOemSupportInfo;
#endif /* !defined(____x_ABI_CWindows_CSystem_CProfile_CSystemManufacturers_CIOemSupportInfo_INTERFACE_DEFINED__) */
#endif // WINDOWS_SYSTEM_PROFILE_SYSTEMMANUFACTURERS_SYSTEMMANUFACTURERSCONTRACT_VERSION >= 0x20000


/*
 *
 * Interface Windows.System.Profile.SystemManufacturers.ISmbiosInformationStatics
 *
 * Introduced to Windows.System.Profile.SystemManufacturers.SystemManufacturersContract in version 1.0
 *
 *
 * Interface is a part of the implementation of type Windows.System.Profile.SystemManufacturers.SmbiosInformation
 *
 *
 */
#if WINDOWS_SYSTEM_PROFILE_SYSTEMMANUFACTURERS_SYSTEMMANUFACTURERSCONTRACT_VERSION >= 0x10000
#if !defined(____x_ABI_CWindows_CSystem_CProfile_CSystemManufacturers_CISmbiosInformationStatics_INTERFACE_DEFINED__)
#define ____x_ABI_CWindows_CSystem_CProfile_CSystemManufacturers_CISmbiosInformationStatics_INTERFACE_DEFINED__
extern const __declspec(selectany) _Null_terminated_ WCHAR InterfaceName_Windows_System_Profile_SystemManufacturers_ISmbiosInformationStatics[] = L"Windows.System.Profile.SystemManufacturers.ISmbiosInformationStatics";
/* [object, uuid("080CCA7C-637C-48C4-B728-F9273812DB8E"), exclusiveto, contract] */
typedef struct __x_ABI_CWindows_CSystem_CProfile_CSystemManufacturers_CISmbiosInformationStaticsVtbl
{
    BEGIN_INTERFACE
    HRESULT ( STDMETHODCALLTYPE *QueryInterface)(
    __RPC__in __x_ABI_CWindows_CSystem_CProfile_CSystemManufacturers_CISmbiosInformationStatics * This,
    /* [in] */ __RPC__in REFIID riid,
    /* [annotation][iid_is][out] */
    _COM_Outptr_  void **ppvObject
    );

ULONG ( STDMETHODCALLTYPE *AddRef )(
    __RPC__in __x_ABI_CWindows_CSystem_CProfile_CSystemManufacturers_CISmbiosInformationStatics * This
    );

ULONG ( STDMETHODCALLTYPE *Release )(
    __RPC__in __x_ABI_CWindows_CSystem_CProfile_CSystemManufacturers_CISmbiosInformationStatics * This
    );

HRESULT ( STDMETHODCALLTYPE *GetIids )(
    __RPC__in __x_ABI_CWindows_CSystem_CProfile_CSystemManufacturers_CISmbiosInformationStatics * This,
    /* [out] */ __RPC__out ULONG *iidCount,
    /* [size_is][size_is][out] */ __RPC__deref_out_ecount_full_opt(*iidCount) IID **iids
    );

HRESULT ( STDMETHODCALLTYPE *GetRuntimeClassName )(
    __RPC__in __x_ABI_CWindows_CSystem_CProfile_CSystemManufacturers_CISmbiosInformationStatics * This,
    /* [out] */ __RPC__deref_out_opt HSTRING *className
    );

HRESULT ( STDMETHODCALLTYPE *GetTrustLevel )(
    __RPC__in __x_ABI_CWindows_CSystem_CProfile_CSystemManufacturers_CISmbiosInformationStatics * This,
    /* [OUT ] */ __RPC__out TrustLevel *trustLevel
    );
/* [propget] */HRESULT ( STDMETHODCALLTYPE *get_SerialNumber )(
        __x_ABI_CWindows_CSystem_CProfile_CSystemManufacturers_CISmbiosInformationStatics * This,
        /* [retval, out] */__RPC__deref_out_opt HSTRING * value
        );
    END_INTERFACE
    
} __x_ABI_CWindows_CSystem_CProfile_CSystemManufacturers_CISmbiosInformationStaticsVtbl;

interface __x_ABI_CWindows_CSystem_CProfile_CSystemManufacturers_CISmbiosInformationStatics
{
    CONST_VTBL struct __x_ABI_CWindows_CSystem_CProfile_CSystemManufacturers_CISmbiosInformationStaticsVtbl *lpVtbl;
};

#ifdef COBJMACROS
#define __x_ABI_CWindows_CSystem_CProfile_CSystemManufacturers_CISmbiosInformationStatics_QueryInterface(This,riid,ppvObject) \
( (This)->lpVtbl->QueryInterface(This,riid,ppvObject) )

#define __x_ABI_CWindows_CSystem_CProfile_CSystemManufacturers_CISmbiosInformationStatics_AddRef(This) \
        ( (This)->lpVtbl->AddRef(This) )

#define __x_ABI_CWindows_CSystem_CProfile_CSystemManufacturers_CISmbiosInformationStatics_Release(This) \
        ( (This)->lpVtbl->Release(This) )

#define __x_ABI_CWindows_CSystem_CProfile_CSystemManufacturers_CISmbiosInformationStatics_GetIids(This,iidCount,iids) \
        ( (This)->lpVtbl->GetIids(This,iidCount,iids) )

#define __x_ABI_CWindows_CSystem_CProfile_CSystemManufacturers_CISmbiosInformationStatics_GetRuntimeClassName(This,className) \
        ( (This)->lpVtbl->GetRuntimeClassName(This,className) )

#define __x_ABI_CWindows_CSystem_CProfile_CSystemManufacturers_CISmbiosInformationStatics_GetTrustLevel(This,trustLevel) \
        ( (This)->lpVtbl->GetTrustLevel(This,trustLevel) )

#define __x_ABI_CWindows_CSystem_CProfile_CSystemManufacturers_CISmbiosInformationStatics_get_SerialNumber(This,value) \
    ( (This)->lpVtbl->get_SerialNumber(This,value) )


#endif /* COBJMACROS */


EXTERN_C const IID IID___x_ABI_CWindows_CSystem_CProfile_CSystemManufacturers_CISmbiosInformationStatics;
#endif /* !defined(____x_ABI_CWindows_CSystem_CProfile_CSystemManufacturers_CISmbiosInformationStatics_INTERFACE_DEFINED__) */
#endif // WINDOWS_SYSTEM_PROFILE_SYSTEMMANUFACTURERS_SYSTEMMANUFACTURERSCONTRACT_VERSION >= 0x10000


/*
 *
 * Interface Windows.System.Profile.SystemManufacturers.ISystemSupportDeviceInfo
 *
 * Introduced to Windows.System.Profile.SystemManufacturers.SystemManufacturersContract in version 3.0
 *
 *
 * Interface is a part of the implementation of type Windows.System.Profile.SystemManufacturers.SystemSupportDeviceInfo
 *
 *
 */
#if WINDOWS_SYSTEM_PROFILE_SYSTEMMANUFACTURERS_SYSTEMMANUFACTURERSCONTRACT_VERSION >= 0x30000
#if !defined(____x_ABI_CWindows_CSystem_CProfile_CSystemManufacturers_CISystemSupportDeviceInfo_INTERFACE_DEFINED__)
#define ____x_ABI_CWindows_CSystem_CProfile_CSystemManufacturers_CISystemSupportDeviceInfo_INTERFACE_DEFINED__
extern const __declspec(selectany) _Null_terminated_ WCHAR InterfaceName_Windows_System_Profile_SystemManufacturers_ISystemSupportDeviceInfo[] = L"Windows.System.Profile.SystemManufacturers.ISystemSupportDeviceInfo";
/* [object, uuid("05880B99-8247-441B-A996-A1784BAB79A8"), exclusiveto, contract] */
typedef struct __x_ABI_CWindows_CSystem_CProfile_CSystemManufacturers_CISystemSupportDeviceInfoVtbl
{
    BEGIN_INTERFACE
    HRESULT ( STDMETHODCALLTYPE *QueryInterface)(
    __RPC__in __x_ABI_CWindows_CSystem_CProfile_CSystemManufacturers_CISystemSupportDeviceInfo * This,
    /* [in] */ __RPC__in REFIID riid,
    /* [annotation][iid_is][out] */
    _COM_Outptr_  void **ppvObject
    );

ULONG ( STDMETHODCALLTYPE *AddRef )(
    __RPC__in __x_ABI_CWindows_CSystem_CProfile_CSystemManufacturers_CISystemSupportDeviceInfo * This
    );

ULONG ( STDMETHODCALLTYPE *Release )(
    __RPC__in __x_ABI_CWindows_CSystem_CProfile_CSystemManufacturers_CISystemSupportDeviceInfo * This
    );

HRESULT ( STDMETHODCALLTYPE *GetIids )(
    __RPC__in __x_ABI_CWindows_CSystem_CProfile_CSystemManufacturers_CISystemSupportDeviceInfo * This,
    /* [out] */ __RPC__out ULONG *iidCount,
    /* [size_is][size_is][out] */ __RPC__deref_out_ecount_full_opt(*iidCount) IID **iids
    );

HRESULT ( STDMETHODCALLTYPE *GetRuntimeClassName )(
    __RPC__in __x_ABI_CWindows_CSystem_CProfile_CSystemManufacturers_CISystemSupportDeviceInfo * This,
    /* [out] */ __RPC__deref_out_opt HSTRING *className
    );

HRESULT ( STDMETHODCALLTYPE *GetTrustLevel )(
    __RPC__in __x_ABI_CWindows_CSystem_CProfile_CSystemManufacturers_CISystemSupportDeviceInfo * This,
    /* [OUT ] */ __RPC__out TrustLevel *trustLevel
    );
/* [propget] */HRESULT ( STDMETHODCALLTYPE *get_OperatingSystem )(
        __x_ABI_CWindows_CSystem_CProfile_CSystemManufacturers_CISystemSupportDeviceInfo * This,
        /* [retval, out] */__RPC__deref_out_opt HSTRING * value
        );
    /* [propget] */HRESULT ( STDMETHODCALLTYPE *get_FriendlyName )(
        __x_ABI_CWindows_CSystem_CProfile_CSystemManufacturers_CISystemSupportDeviceInfo * This,
        /* [retval, out] */__RPC__deref_out_opt HSTRING * value
        );
    /* [propget] */HRESULT ( STDMETHODCALLTYPE *get_SystemManufacturer )(
        __x_ABI_CWindows_CSystem_CProfile_CSystemManufacturers_CISystemSupportDeviceInfo * This,
        /* [retval, out] */__RPC__deref_out_opt HSTRING * value
        );
    /* [propget] */HRESULT ( STDMETHODCALLTYPE *get_SystemProductName )(
        __x_ABI_CWindows_CSystem_CProfile_CSystemManufacturers_CISystemSupportDeviceInfo * This,
        /* [retval, out] */__RPC__deref_out_opt HSTRING * value
        );
    /* [propget] */HRESULT ( STDMETHODCALLTYPE *get_SystemSku )(
        __x_ABI_CWindows_CSystem_CProfile_CSystemManufacturers_CISystemSupportDeviceInfo * This,
        /* [retval, out] */__RPC__deref_out_opt HSTRING * value
        );
    /* [propget] */HRESULT ( STDMETHODCALLTYPE *get_SystemHardwareVersion )(
        __x_ABI_CWindows_CSystem_CProfile_CSystemManufacturers_CISystemSupportDeviceInfo * This,
        /* [retval, out] */__RPC__deref_out_opt HSTRING * value
        );
    /* [propget] */HRESULT ( STDMETHODCALLTYPE *get_SystemFirmwareVersion )(
        __x_ABI_CWindows_CSystem_CProfile_CSystemManufacturers_CISystemSupportDeviceInfo * This,
        /* [retval, out] */__RPC__deref_out_opt HSTRING * value
        );
    END_INTERFACE
    
} __x_ABI_CWindows_CSystem_CProfile_CSystemManufacturers_CISystemSupportDeviceInfoVtbl;

interface __x_ABI_CWindows_CSystem_CProfile_CSystemManufacturers_CISystemSupportDeviceInfo
{
    CONST_VTBL struct __x_ABI_CWindows_CSystem_CProfile_CSystemManufacturers_CISystemSupportDeviceInfoVtbl *lpVtbl;
};

#ifdef COBJMACROS
#define __x_ABI_CWindows_CSystem_CProfile_CSystemManufacturers_CISystemSupportDeviceInfo_QueryInterface(This,riid,ppvObject) \
( (This)->lpVtbl->QueryInterface(This,riid,ppvObject) )

#define __x_ABI_CWindows_CSystem_CProfile_CSystemManufacturers_CISystemSupportDeviceInfo_AddRef(This) \
        ( (This)->lpVtbl->AddRef(This) )

#define __x_ABI_CWindows_CSystem_CProfile_CSystemManufacturers_CISystemSupportDeviceInfo_Release(This) \
        ( (This)->lpVtbl->Release(This) )

#define __x_ABI_CWindows_CSystem_CProfile_CSystemManufacturers_CISystemSupportDeviceInfo_GetIids(This,iidCount,iids) \
        ( (This)->lpVtbl->GetIids(This,iidCount,iids) )

#define __x_ABI_CWindows_CSystem_CProfile_CSystemManufacturers_CISystemSupportDeviceInfo_GetRuntimeClassName(This,className) \
        ( (This)->lpVtbl->GetRuntimeClassName(This,className) )

#define __x_ABI_CWindows_CSystem_CProfile_CSystemManufacturers_CISystemSupportDeviceInfo_GetTrustLevel(This,trustLevel) \
        ( (This)->lpVtbl->GetTrustLevel(This,trustLevel) )

#define __x_ABI_CWindows_CSystem_CProfile_CSystemManufacturers_CISystemSupportDeviceInfo_get_OperatingSystem(This,value) \
    ( (This)->lpVtbl->get_OperatingSystem(This,value) )

#define __x_ABI_CWindows_CSystem_CProfile_CSystemManufacturers_CISystemSupportDeviceInfo_get_FriendlyName(This,value) \
    ( (This)->lpVtbl->get_FriendlyName(This,value) )

#define __x_ABI_CWindows_CSystem_CProfile_CSystemManufacturers_CISystemSupportDeviceInfo_get_SystemManufacturer(This,value) \
    ( (This)->lpVtbl->get_SystemManufacturer(This,value) )

#define __x_ABI_CWindows_CSystem_CProfile_CSystemManufacturers_CISystemSupportDeviceInfo_get_SystemProductName(This,value) \
    ( (This)->lpVtbl->get_SystemProductName(This,value) )

#define __x_ABI_CWindows_CSystem_CProfile_CSystemManufacturers_CISystemSupportDeviceInfo_get_SystemSku(This,value) \
    ( (This)->lpVtbl->get_SystemSku(This,value) )

#define __x_ABI_CWindows_CSystem_CProfile_CSystemManufacturers_CISystemSupportDeviceInfo_get_SystemHardwareVersion(This,value) \
    ( (This)->lpVtbl->get_SystemHardwareVersion(This,value) )

#define __x_ABI_CWindows_CSystem_CProfile_CSystemManufacturers_CISystemSupportDeviceInfo_get_SystemFirmwareVersion(This,value) \
    ( (This)->lpVtbl->get_SystemFirmwareVersion(This,value) )


#endif /* COBJMACROS */


EXTERN_C const IID IID___x_ABI_CWindows_CSystem_CProfile_CSystemManufacturers_CISystemSupportDeviceInfo;
#endif /* !defined(____x_ABI_CWindows_CSystem_CProfile_CSystemManufacturers_CISystemSupportDeviceInfo_INTERFACE_DEFINED__) */
#endif // WINDOWS_SYSTEM_PROFILE_SYSTEMMANUFACTURERS_SYSTEMMANUFACTURERSCONTRACT_VERSION >= 0x30000


/*
 *
 * Interface Windows.System.Profile.SystemManufacturers.ISystemSupportInfoStatics
 *
 * Introduced to Windows.System.Profile.SystemManufacturers.SystemManufacturersContract in version 2.0
 *
 *
 * Interface is a part of the implementation of type Windows.System.Profile.SystemManufacturers.SystemSupportInfo
 *
 *
 */
#if WINDOWS_SYSTEM_PROFILE_SYSTEMMANUFACTURERS_SYSTEMMANUFACTURERSCONTRACT_VERSION >= 0x20000
#if !defined(____x_ABI_CWindows_CSystem_CProfile_CSystemManufacturers_CISystemSupportInfoStatics_INTERFACE_DEFINED__)
#define ____x_ABI_CWindows_CSystem_CProfile_CSystemManufacturers_CISystemSupportInfoStatics_INTERFACE_DEFINED__
extern const __declspec(selectany) _Null_terminated_ WCHAR InterfaceName_Windows_System_Profile_SystemManufacturers_ISystemSupportInfoStatics[] = L"Windows.System.Profile.SystemManufacturers.ISystemSupportInfoStatics";
/* [object, uuid("EF750974-C422-45D7-A44D-5C1C0043A2B3"), exclusiveto, contract] */
typedef struct __x_ABI_CWindows_CSystem_CProfile_CSystemManufacturers_CISystemSupportInfoStaticsVtbl
{
    BEGIN_INTERFACE
    HRESULT ( STDMETHODCALLTYPE *QueryInterface)(
    __RPC__in __x_ABI_CWindows_CSystem_CProfile_CSystemManufacturers_CISystemSupportInfoStatics * This,
    /* [in] */ __RPC__in REFIID riid,
    /* [annotation][iid_is][out] */
    _COM_Outptr_  void **ppvObject
    );

ULONG ( STDMETHODCALLTYPE *AddRef )(
    __RPC__in __x_ABI_CWindows_CSystem_CProfile_CSystemManufacturers_CISystemSupportInfoStatics * This
    );

ULONG ( STDMETHODCALLTYPE *Release )(
    __RPC__in __x_ABI_CWindows_CSystem_CProfile_CSystemManufacturers_CISystemSupportInfoStatics * This
    );

HRESULT ( STDMETHODCALLTYPE *GetIids )(
    __RPC__in __x_ABI_CWindows_CSystem_CProfile_CSystemManufacturers_CISystemSupportInfoStatics * This,
    /* [out] */ __RPC__out ULONG *iidCount,
    /* [size_is][size_is][out] */ __RPC__deref_out_ecount_full_opt(*iidCount) IID **iids
    );

HRESULT ( STDMETHODCALLTYPE *GetRuntimeClassName )(
    __RPC__in __x_ABI_CWindows_CSystem_CProfile_CSystemManufacturers_CISystemSupportInfoStatics * This,
    /* [out] */ __RPC__deref_out_opt HSTRING *className
    );

HRESULT ( STDMETHODCALLTYPE *GetTrustLevel )(
    __RPC__in __x_ABI_CWindows_CSystem_CProfile_CSystemManufacturers_CISystemSupportInfoStatics * This,
    /* [OUT ] */ __RPC__out TrustLevel *trustLevel
    );
/* [propget] */HRESULT ( STDMETHODCALLTYPE *get_LocalSystemEdition )(
        __x_ABI_CWindows_CSystem_CProfile_CSystemManufacturers_CISystemSupportInfoStatics * This,
        /* [retval, out] */__RPC__deref_out_opt HSTRING * value
        );
    /* [propget] */HRESULT ( STDMETHODCALLTYPE *get_OemSupportInfo )(
        __x_ABI_CWindows_CSystem_CProfile_CSystemManufacturers_CISystemSupportInfoStatics * This,
        /* [retval, out] */__RPC__deref_out_opt __x_ABI_CWindows_CSystem_CProfile_CSystemManufacturers_CIOemSupportInfo * * value
        );
    END_INTERFACE
    
} __x_ABI_CWindows_CSystem_CProfile_CSystemManufacturers_CISystemSupportInfoStaticsVtbl;

interface __x_ABI_CWindows_CSystem_CProfile_CSystemManufacturers_CISystemSupportInfoStatics
{
    CONST_VTBL struct __x_ABI_CWindows_CSystem_CProfile_CSystemManufacturers_CISystemSupportInfoStaticsVtbl *lpVtbl;
};

#ifdef COBJMACROS
#define __x_ABI_CWindows_CSystem_CProfile_CSystemManufacturers_CISystemSupportInfoStatics_QueryInterface(This,riid,ppvObject) \
( (This)->lpVtbl->QueryInterface(This,riid,ppvObject) )

#define __x_ABI_CWindows_CSystem_CProfile_CSystemManufacturers_CISystemSupportInfoStatics_AddRef(This) \
        ( (This)->lpVtbl->AddRef(This) )

#define __x_ABI_CWindows_CSystem_CProfile_CSystemManufacturers_CISystemSupportInfoStatics_Release(This) \
        ( (This)->lpVtbl->Release(This) )

#define __x_ABI_CWindows_CSystem_CProfile_CSystemManufacturers_CISystemSupportInfoStatics_GetIids(This,iidCount,iids) \
        ( (This)->lpVtbl->GetIids(This,iidCount,iids) )

#define __x_ABI_CWindows_CSystem_CProfile_CSystemManufacturers_CISystemSupportInfoStatics_GetRuntimeClassName(This,className) \
        ( (This)->lpVtbl->GetRuntimeClassName(This,className) )

#define __x_ABI_CWindows_CSystem_CProfile_CSystemManufacturers_CISystemSupportInfoStatics_GetTrustLevel(This,trustLevel) \
        ( (This)->lpVtbl->GetTrustLevel(This,trustLevel) )

#define __x_ABI_CWindows_CSystem_CProfile_CSystemManufacturers_CISystemSupportInfoStatics_get_LocalSystemEdition(This,value) \
    ( (This)->lpVtbl->get_LocalSystemEdition(This,value) )

#define __x_ABI_CWindows_CSystem_CProfile_CSystemManufacturers_CISystemSupportInfoStatics_get_OemSupportInfo(This,value) \
    ( (This)->lpVtbl->get_OemSupportInfo(This,value) )


#endif /* COBJMACROS */


EXTERN_C const IID IID___x_ABI_CWindows_CSystem_CProfile_CSystemManufacturers_CISystemSupportInfoStatics;
#endif /* !defined(____x_ABI_CWindows_CSystem_CProfile_CSystemManufacturers_CISystemSupportInfoStatics_INTERFACE_DEFINED__) */
#endif // WINDOWS_SYSTEM_PROFILE_SYSTEMMANUFACTURERS_SYSTEMMANUFACTURERSCONTRACT_VERSION >= 0x20000


/*
 *
 * Interface Windows.System.Profile.SystemManufacturers.ISystemSupportInfoStatics2
 *
 * Introduced to Windows.System.Profile.SystemManufacturers.SystemManufacturersContract in version 3.0
 *
 *
 * Interface is a part of the implementation of type Windows.System.Profile.SystemManufacturers.SystemSupportInfo
 *
 *
 */
#if WINDOWS_SYSTEM_PROFILE_SYSTEMMANUFACTURERS_SYSTEMMANUFACTURERSCONTRACT_VERSION >= 0x30000
#if !defined(____x_ABI_CWindows_CSystem_CProfile_CSystemManufacturers_CISystemSupportInfoStatics2_INTERFACE_DEFINED__)
#define ____x_ABI_CWindows_CSystem_CProfile_CSystemManufacturers_CISystemSupportInfoStatics2_INTERFACE_DEFINED__
extern const __declspec(selectany) _Null_terminated_ WCHAR InterfaceName_Windows_System_Profile_SystemManufacturers_ISystemSupportInfoStatics2[] = L"Windows.System.Profile.SystemManufacturers.ISystemSupportInfoStatics2";
/* [object, uuid("33F349A4-3FA1-4986-AA4B-057420455E6D"), exclusiveto, contract] */
typedef struct __x_ABI_CWindows_CSystem_CProfile_CSystemManufacturers_CISystemSupportInfoStatics2Vtbl
{
    BEGIN_INTERFACE
    HRESULT ( STDMETHODCALLTYPE *QueryInterface)(
    __RPC__in __x_ABI_CWindows_CSystem_CProfile_CSystemManufacturers_CISystemSupportInfoStatics2 * This,
    /* [in] */ __RPC__in REFIID riid,
    /* [annotation][iid_is][out] */
    _COM_Outptr_  void **ppvObject
    );

ULONG ( STDMETHODCALLTYPE *AddRef )(
    __RPC__in __x_ABI_CWindows_CSystem_CProfile_CSystemManufacturers_CISystemSupportInfoStatics2 * This
    );

ULONG ( STDMETHODCALLTYPE *Release )(
    __RPC__in __x_ABI_CWindows_CSystem_CProfile_CSystemManufacturers_CISystemSupportInfoStatics2 * This
    );

HRESULT ( STDMETHODCALLTYPE *GetIids )(
    __RPC__in __x_ABI_CWindows_CSystem_CProfile_CSystemManufacturers_CISystemSupportInfoStatics2 * This,
    /* [out] */ __RPC__out ULONG *iidCount,
    /* [size_is][size_is][out] */ __RPC__deref_out_ecount_full_opt(*iidCount) IID **iids
    );

HRESULT ( STDMETHODCALLTYPE *GetRuntimeClassName )(
    __RPC__in __x_ABI_CWindows_CSystem_CProfile_CSystemManufacturers_CISystemSupportInfoStatics2 * This,
    /* [out] */ __RPC__deref_out_opt HSTRING *className
    );

HRESULT ( STDMETHODCALLTYPE *GetTrustLevel )(
    __RPC__in __x_ABI_CWindows_CSystem_CProfile_CSystemManufacturers_CISystemSupportInfoStatics2 * This,
    /* [OUT ] */ __RPC__out TrustLevel *trustLevel
    );
/* [propget] */HRESULT ( STDMETHODCALLTYPE *get_LocalDeviceInfo )(
        __x_ABI_CWindows_CSystem_CProfile_CSystemManufacturers_CISystemSupportInfoStatics2 * This,
        /* [retval, out] */__RPC__deref_out_opt __x_ABI_CWindows_CSystem_CProfile_CSystemManufacturers_CISystemSupportDeviceInfo * * value
        );
    END_INTERFACE
    
} __x_ABI_CWindows_CSystem_CProfile_CSystemManufacturers_CISystemSupportInfoStatics2Vtbl;

interface __x_ABI_CWindows_CSystem_CProfile_CSystemManufacturers_CISystemSupportInfoStatics2
{
    CONST_VTBL struct __x_ABI_CWindows_CSystem_CProfile_CSystemManufacturers_CISystemSupportInfoStatics2Vtbl *lpVtbl;
};

#ifdef COBJMACROS
#define __x_ABI_CWindows_CSystem_CProfile_CSystemManufacturers_CISystemSupportInfoStatics2_QueryInterface(This,riid,ppvObject) \
( (This)->lpVtbl->QueryInterface(This,riid,ppvObject) )

#define __x_ABI_CWindows_CSystem_CProfile_CSystemManufacturers_CISystemSupportInfoStatics2_AddRef(This) \
        ( (This)->lpVtbl->AddRef(This) )

#define __x_ABI_CWindows_CSystem_CProfile_CSystemManufacturers_CISystemSupportInfoStatics2_Release(This) \
        ( (This)->lpVtbl->Release(This) )

#define __x_ABI_CWindows_CSystem_CProfile_CSystemManufacturers_CISystemSupportInfoStatics2_GetIids(This,iidCount,iids) \
        ( (This)->lpVtbl->GetIids(This,iidCount,iids) )

#define __x_ABI_CWindows_CSystem_CProfile_CSystemManufacturers_CISystemSupportInfoStatics2_GetRuntimeClassName(This,className) \
        ( (This)->lpVtbl->GetRuntimeClassName(This,className) )

#define __x_ABI_CWindows_CSystem_CProfile_CSystemManufacturers_CISystemSupportInfoStatics2_GetTrustLevel(This,trustLevel) \
        ( (This)->lpVtbl->GetTrustLevel(This,trustLevel) )

#define __x_ABI_CWindows_CSystem_CProfile_CSystemManufacturers_CISystemSupportInfoStatics2_get_LocalDeviceInfo(This,value) \
    ( (This)->lpVtbl->get_LocalDeviceInfo(This,value) )


#endif /* COBJMACROS */


EXTERN_C const IID IID___x_ABI_CWindows_CSystem_CProfile_CSystemManufacturers_CISystemSupportInfoStatics2;
#endif /* !defined(____x_ABI_CWindows_CSystem_CProfile_CSystemManufacturers_CISystemSupportInfoStatics2_INTERFACE_DEFINED__) */
#endif // WINDOWS_SYSTEM_PROFILE_SYSTEMMANUFACTURERS_SYSTEMMANUFACTURERSCONTRACT_VERSION >= 0x30000


/*
 *
 * Class Windows.System.Profile.SystemManufacturers.OemSupportInfo
 *
 * Introduced to Windows.System.Profile.SystemManufacturers.SystemManufacturersContract in version 2.0
 *
 *
 * Class implements the following interfaces:
 *    Windows.System.Profile.SystemManufacturers.IOemSupportInfo ** Default Interface **
 *
 * Class Threading Model:  Both Single and Multi Threaded Apartment
 *
 * Class Marshaling Behavior:  Agile - Class is agile
 *
 */
#if WINDOWS_SYSTEM_PROFILE_SYSTEMMANUFACTURERS_SYSTEMMANUFACTURERSCONTRACT_VERSION >= 0x20000

#ifndef RUNTIMECLASS_Windows_System_Profile_SystemManufacturers_OemSupportInfo_DEFINED
#define RUNTIMECLASS_Windows_System_Profile_SystemManufacturers_OemSupportInfo_DEFINED
extern const __declspec(selectany) _Null_terminated_ WCHAR RuntimeClass_Windows_System_Profile_SystemManufacturers_OemSupportInfo[] = L"Windows.System.Profile.SystemManufacturers.OemSupportInfo";
#endif
#endif // WINDOWS_SYSTEM_PROFILE_SYSTEMMANUFACTURERS_SYSTEMMANUFACTURERSCONTRACT_VERSION >= 0x20000


/*
 *
 * Class Windows.System.Profile.SystemManufacturers.SmbiosInformation
 *
 * Introduced to Windows.System.Profile.SystemManufacturers.SystemManufacturersContract in version 1.0
 *
 *
 * RuntimeClass contains static methods.
 *   Static Methods exist on the Windows.System.Profile.SystemManufacturers.ISmbiosInformationStatics interface starting with version 1.0 of the Windows.System.Profile.SystemManufacturers.SystemManufacturersContract API contract
 *
 * Class Marshaling Behavior:  Agile - Class is agile
 *
 */
#if WINDOWS_SYSTEM_PROFILE_SYSTEMMANUFACTURERS_SYSTEMMANUFACTURERSCONTRACT_VERSION >= 0x10000
#ifndef RUNTIMECLASS_Windows_System_Profile_SystemManufacturers_SmbiosInformation_DEFINED
#define RUNTIMECLASS_Windows_System_Profile_SystemManufacturers_SmbiosInformation_DEFINED
extern const __declspec(selectany) _Null_terminated_ WCHAR RuntimeClass_Windows_System_Profile_SystemManufacturers_SmbiosInformation[] = L"Windows.System.Profile.SystemManufacturers.SmbiosInformation";
#endif
#endif // WINDOWS_SYSTEM_PROFILE_SYSTEMMANUFACTURERS_SYSTEMMANUFACTURERSCONTRACT_VERSION >= 0x10000


/*
 *
 * Class Windows.System.Profile.SystemManufacturers.SystemSupportDeviceInfo
 *
 * Introduced to Windows.System.Profile.SystemManufacturers.SystemManufacturersContract in version 3.0
 *
 *
 * Class implements the following interfaces:
 *    Windows.System.Profile.SystemManufacturers.ISystemSupportDeviceInfo ** Default Interface **
 *
 * Class Threading Model:  Both Single and Multi Threaded Apartment
 *
 * Class Marshaling Behavior:  Agile - Class is agile
 *
 */
#if WINDOWS_SYSTEM_PROFILE_SYSTEMMANUFACTURERS_SYSTEMMANUFACTURERSCONTRACT_VERSION >= 0x30000

#ifndef RUNTIMECLASS_Windows_System_Profile_SystemManufacturers_SystemSupportDeviceInfo_DEFINED
#define RUNTIMECLASS_Windows_System_Profile_SystemManufacturers_SystemSupportDeviceInfo_DEFINED
extern const __declspec(selectany) _Null_terminated_ WCHAR RuntimeClass_Windows_System_Profile_SystemManufacturers_SystemSupportDeviceInfo[] = L"Windows.System.Profile.SystemManufacturers.SystemSupportDeviceInfo";
#endif
#endif // WINDOWS_SYSTEM_PROFILE_SYSTEMMANUFACTURERS_SYSTEMMANUFACTURERSCONTRACT_VERSION >= 0x30000


/*
 *
 * Class Windows.System.Profile.SystemManufacturers.SystemSupportInfo
 *
 * Introduced to Windows.System.Profile.SystemManufacturers.SystemManufacturersContract in version 2.0
 *
 *
 * RuntimeClass contains static methods.
 *   Static Methods exist on the Windows.System.Profile.SystemManufacturers.ISystemSupportInfoStatics2 interface starting with version 3.0 of the Windows.System.Profile.SystemManufacturers.SystemManufacturersContract API contract
 *   Static Methods exist on the Windows.System.Profile.SystemManufacturers.ISystemSupportInfoStatics interface starting with version 2.0 of the Windows.System.Profile.SystemManufacturers.SystemManufacturersContract API contract
 *
 * Class Threading Model:  Both Single and Multi Threaded Apartment
 *
 * Class Marshaling Behavior:  Agile - Class is agile
 *
 */
#if WINDOWS_SYSTEM_PROFILE_SYSTEMMANUFACTURERS_SYSTEMMANUFACTURERSCONTRACT_VERSION >= 0x20000
#ifndef RUNTIMECLASS_Windows_System_Profile_SystemManufacturers_SystemSupportInfo_DEFINED
#define RUNTIMECLASS_Windows_System_Profile_SystemManufacturers_SystemSupportInfo_DEFINED
extern const __declspec(selectany) _Null_terminated_ WCHAR RuntimeClass_Windows_System_Profile_SystemManufacturers_SystemSupportInfo[] = L"Windows.System.Profile.SystemManufacturers.SystemSupportInfo";
#endif
#endif // WINDOWS_SYSTEM_PROFILE_SYSTEMMANUFACTURERS_SYSTEMMANUFACTURERSCONTRACT_VERSION >= 0x20000





#endif // defined(__cplusplus)
#pragma pop_macro("MIDL_CONST_ID")
// Restore the original value of the 'DEPRECATED' macro
#pragma pop_macro("DEPRECATED")

#ifdef __clang__
#pragma clang diagnostic pop // deprecated-declarations
#else
#pragma warning(pop)
#endif
#endif // __windows2Esystem2Eprofile2Esystemmanufacturers_p_h__

#endif // __windows2Esystem2Eprofile2Esystemmanufacturers_h__
