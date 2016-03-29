/*
* Copyright (c) 2013-2016, The PurpleI2P Project
*
* This file is part of Purple i2pd project and licensed under BSD3
*
* See full license text in LICENSE file at top of project tree
*/
#ifndef DIALOGTEMPLATEHELPER_H__
#define DIALOGTEMPLATEHELPER_H__

namespace i2p
{
	namespace win32
	{
		// tweak for sizeof
#pragma pack(push, 1)
		/**
		* Copy-pasted from https://msdn.microsoft.com/en-us/library/ms645398%28v=vs.85%29.aspx
		*/
		typedef struct {
			WORD      dlgVer;
			WORD      signature;
			DWORD     helpID;
			DWORD     dwExtendedStyle;
			DWORD     style;
			WORD      cdit;
			short     x;
			short     y;
			short     cx;
			short     cy;
			//sz_Or_Ord menu;
			//sz_Or_Ord windowClass;
			//WCHAR     title[titleLen];
			//WORD      pointsize;
			//WORD      weight;
			//BYTE      italic;
			//BYTE      charset;
			//WCHAR     typeface[stringLen];
		} DLGTEMPLATEEX, *LPDLGTEMPLATEEX;

		/**
		 * Copy-pasted from https://msdn.microsoft.com/en-us/library/ms645389%28v=vs.85%29.aspx
		 */
		typedef struct {
			DWORD     helpID;
			DWORD     dwExtendedStyle;
			DWORD     style;
			short     x;
			short     y;
			short     cx;
			short     cy;
			DWORD     id;
			//sz_Or_Ord windowClass;
			//sz_Or_Ord title;
			//WORD      extraCount;
		} DLGITEMTEMPLATEEX, *LPDLGITEMTEMPLATEEX;
#pragma pack(pop)

		/**
		 * Helps to build dialog template in memory on-the-fly
		 * using either regular or extended dialogs.
		 *
		 * User is responsible to be consitent regarding regular or extended stuff.
		 * https://msdn.microsoft.com/en-us/library/ms644994%28v=vs.85%29.aspx#templates_in_memory
		 */
		class DialogTemplateHelper {
		public:
			DialogTemplateHelper() {
				// DWORD aligned
				heap = _aligned_malloc(increment, 4);
				if (NULL == heap)
					throw std::runtime_error("Failed to allocate");
				capacity = increment;
			}
			template<class T>
			T* Allocate() {
				return static_cast<T*>( Allocate(sizeof(T)) );
			}
			template<class T>
			T* Allocate0() {
				T* ptr = static_cast<T*>( Allocate(sizeof(T)) );
				memset(ptr, 0, sizeof(T));
				return ptr;
			}
			void* Allocate(size_t amount) {
				if (size + amount > capacity) {
					capacity = ((size + amount) / increment + 1) * increment;
					heap = _aligned_realloc(heap, capacity, 4);
					if (NULL == heap)
						throw std::runtime_error("Failed to reallocate");
				}
				void* ptr = static_cast<char*>(heap) + size;
				size += amount;
				return ptr;
			}
			void* Allocate0(size_t amount) {
				void* ptr = Allocate(amount);
				memset(ptr, 0, amount);
				return ptr;
			}
			template<class T>
			void Align() {
				size_t gap = size % sizeof(T);
				if (gap > 0)
					Allocate0(sizeof(T) - gap);
			}
			/**
			 * Convert up to cbMultiByte bytes (or up to null whichever is smaller)
			 * from str string to wide chars. Result is always WCHAR null-terminated.
			 */
			void Write(const char* str, int cbMultiByte = -1) {
				if (cbMultiByte > 0 && cbMultiByte != strnlen(str, cbMultiByte))
					cbMultiByte = -1;
				//size_t len = strlen(str);
				//char* ptr = (char*)Allocate(len + 1);
				//strncpy(ptr, str, len);
				int len = MultiByteToWideChar(CP_ACP, 0, str, cbMultiByte, NULL, 0);
				LPWSTR lpwsz = static_cast<LPWSTR>( Allocate(len * sizeof(WCHAR)) );
				MultiByteToWideChar(CP_ACP, 0, str, cbMultiByte, lpwsz, len);
				if (cbMultiByte > 0) // force null termination
					*Allocate<WORD>() = 0;
			}
			bool IsExtended() {
				LPWORD ptr = static_cast<LPWORD>(heap);
				return (1 == ptr[0]) && (0xFFFF == ptr[1]);
			}
			/**
			 * Pass result to indirect dialog creation WinAPI functions.
			 *
			 * Use right before dialog creation function
			 * as saved pointer might become invalid due to reallocation.
			 * This will also update dialog items count.
			 */
			LPCDLGTEMPLATE GetBuffer() {
				if (IsExtended())
					static_cast<LPDLGTEMPLATEEX>(heap)->cdit = items;
				else
					static_cast<LPDLGTEMPLATE>(heap)->cdit = items;
				return static_cast<LPCDLGTEMPLATE>(heap);
			}
			//LPDLGTEMPLATEEX GetBufferEx() {
			//	return (LPDLGTEMPLATEEX)heap;
			//}
			~DialogTemplateHelper() {
				if (NULL != heap)
					_aligned_free(heap);
			}
		private:
			size_t capacity = 0;
			size_t size = 0;
			WORD items = 0;
			const size_t increment = 10240;
			void* heap = NULL;
		};

		template<>
		LPDLGITEMTEMPLATE DialogTemplateHelper::Allocate<DLGITEMTEMPLATE>() {
			Align<DWORD>();
			LPDLGITEMTEMPLATE ptr = static_cast<LPDLGITEMTEMPLATE>(Allocate(sizeof(DLGITEMTEMPLATE)));
			items++;
			return ptr;
		}

		template<>
		LPDLGITEMTEMPLATEEX DialogTemplateHelper::Allocate<DLGITEMTEMPLATEEX>() {
			Align<DWORD>();
			LPDLGITEMTEMPLATEEX ptr = static_cast<LPDLGITEMTEMPLATEEX>(Allocate(sizeof(DLGITEMTEMPLATEEX)));
			items++;
			return ptr;
		}
	}
}

#endif
