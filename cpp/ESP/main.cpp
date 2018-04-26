#include <windows.h>
#include <TlHelp32.h>
#include <iostream>

typedef unsigned char uint8_t;

template <typename T, size_t N>

size_t countof(T(&array)[N])
{
	return N;
}

DWORD dwLocalPlayer; //will be scanned
DWORD dwEntityList;  //will be scanned
DWORD dwGlow;        //will be scanned

DWORD dwTeam = 0xF0;
DWORD dwDormant = 0xE9;

struct PModule
{
	DWORD dwBase;
	DWORD dwSize;
};

/* Debugger/Process API implementation class */
class process
{

public:
	bool Attach(char* pName, DWORD rights)
	{
		HANDLE handle = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, NULL);
		PROCESSENTRY32 entry;
		entry.dwSize = sizeof(entry);

		do
			if (!strcmp(entry.szExeFile, pName)) {
				pID = entry.th32ProcessID;
				CloseHandle(handle);
				_process = OpenProcess(rights, false, pID);
				return true;
			}
		while (Process32Next(handle, &entry));
		return false;
	}
	PModule GetModule(char* moduleName) {
		HANDLE module = CreateToolhelp32Snapshot(TH32CS_SNAPMODULE, pID);
		MODULEENTRY32 mEntry;
		mEntry.dwSize = sizeof(mEntry);

		do {
			if (!strcmp(mEntry.szModule, (LPSTR)moduleName)) {
				CloseHandle(module);

				PModule mod = { (DWORD)mEntry.hModule, mEntry.modBaseSize };
				return mod;
			}
		} while (Module32Next(module, &mEntry));

		PModule mod = { (DWORD)false, (DWORD)false };
		return mod;
	}

	template <class T>
	T Read(DWORD addr) {
		T _read;
		ReadProcessMemory(_process, (LPVOID)addr, &_read, sizeof(T), NULL);
		return _read;
	}
	template <class T>
	void Write(DWORD addr, T val) {
		WriteProcessMemory(_process, (LPVOID)addr, &val, sizeof(T), NULL);
	}

	DWORD FindPattern(DWORD start, DWORD size, const char* sig, const char* mask) {
		BYTE* data = new BYTE[size];

		unsigned long bytesRead;
		if (!ReadProcessMemory(_process, (LPVOID)start, data, size, &bytesRead)) {
			return NULL;
		}

		for (DWORD i = 0; i < size; i++) {
			if (DataCompare((const BYTE*)(data + i), (const BYTE*)sig, mask)) {
				return start + i;
			}
		}
		return NULL;
	}

	DWORD FindPatternArray(DWORD start, DWORD size, const char* mask, int count, ...) {
		char* sig = new char[count + 1];
		va_list ap;
		va_start(ap, count);
		for (int i = 0; i < count; i++) {
			char read = va_arg(ap, char);
			sig[i] = read;
		}
		va_end(ap);
		sig[count] = '\0';
		return FindPattern(start, size, sig, mask);
	}


private:
	HANDLE _process;
	DWORD pID;
	bool DataCompare(const BYTE* pData, const BYTE* pMask, const char* pszMask) {
		for (; *pszMask; ++pszMask, ++pData, ++pMask) {
			if (*pszMask == 'x' && *pData != *pMask) {
				return false;
			}
		}
		return (*pszMask == NULL);
	}
};

/* Glow Object structure in csgo */
struct glow_t
{
	DWORD dwBase;
	float r;
	float g;
	float b;
	float a;
	uint8_t unk1[16];
	bool m_bRenderWhenOccluded;
	bool m_bRenderWhenUnoccluded;
	bool m_bFullBloom;
	uint8_t unk2[10];
};

/* Entity structure in csgo */
struct Entity
{
	DWORD dwBase;
	int team;
	bool is_dormant;
};

/* Player structure in csgo */
struct Player
{
	DWORD dwBase;
	bool isDormant;
};

process memory;
process _modClient;
process* mem;
PModule modClient;

int iFriendlies;
int iEnemies;

Entity entEnemies[32];
Entity entFriendlies[32];
Entity me;

void update_entity_data(Entity* e, DWORD dwBase)
{
	int dormant = memory.Read<int>(dwBase + dwDormant);
	e->dwBase = dwBase;
	e->team = memory.Read<int>(dwBase + dwTeam);
	e->is_dormant = dormant == 1;
}
/* Get Pointer To Client.dll*/
PModule* GetClientModule() {
	if (modClient.dwBase == 0 && modClient.dwSize == 0) {
		modClient = memory.GetModule("client.dll");
	}
	return &modClient;
}

Entity* GetEntityByBase(DWORD dwBase) {

	for (int i = 0; i < iFriendlies; i++) {
		if (dwBase == entFriendlies[i].dwBase) {
			return &entFriendlies[i];
		}
	}
	for (int i = 0; i < iEnemies; i++) {
		if (dwBase == entEnemies[i].dwBase) {
			return &entEnemies[i];
		}
	}
	return nullptr;
}

/* offset updating class, that uses patterns to find memory addresses */
class offset
{
private:
	static void update_local_player() {
		DWORD lpStart = mem->FindPatternArray(modClient.dwBase, modClient.dwSize, "xxx????xx????xxxxx?", 19, 0x8D, 0x34, 0x85, 0x0, 0x0, 0x0, 0x0, 0x89, 0x15, 0x0, 0x0, 0x0, 0x0, 0x8B, 0x41, 0x8, 0x8B, 0x48, 0x0);
		DWORD lpP1 = mem->Read<DWORD>(lpStart + 3);
		BYTE lpP2 = mem->Read<BYTE>(lpStart + 18);
		dwLocalPlayer = (lpP1 + lpP2) - modClient.dwBase;
	}

	static void update_entity_list() {
		DWORD elStart = mem->FindPatternArray(modClient.dwBase, modClient.dwSize, "x????xx?xxx", 11, 0x5, 0x0, 0x0, 0x0, 0x0, 0xC1, 0xE9, 0x0, 0x39, 0x48, 0x4);
		DWORD elP1 = mem->Read<DWORD>(elStart + 1);
		BYTE elP2 = mem->Read<BYTE>(elStart + 7);
		dwEntityList = (elP1 + elP2) - modClient.dwBase;
	}

	static void update_glow() {
		DWORD gpStart = mem->FindPatternArray(modClient.dwBase, modClient.dwSize, "xx????x????xxx????xx????xx", 27, 0x8D, 0x8F, 0, 0, 0, 0, 0xA1, 0, 0, 0, 0, 0xC7, 0x4, 0x2, 0, 0, 0, 0, 0x89, 0x35, 0x0, 0x0, 0x0, 0x0, 0x8B, 0x51);
		dwGlow = mem->Read<DWORD>(gpStart + 7) - modClient.dwBase;
	}
public:
	static void get_offset(process* m) {
		mem = m;
		modClient = mem->GetModule("client.dll");
		update_local_player();
		update_entity_list();
		update_glow();
	}

	//constantly scanning & updating our offsets
	static DWORD WINAPI scan_offsets(LPVOID PARAM)
	{
		Entity players[64];
		while (true) {
			DWORD playerBase = memory.Read<DWORD>(GetClientModule()->dwBase + dwLocalPlayer);
			int cp = 0;

			update_entity_data(&me, playerBase);
			for (int i = 1; i < 64; i++) {
				DWORD entBase = memory.Read<DWORD>((GetClientModule()->dwBase + dwEntityList) + i * 0x10);

				if (entBase == NULL)
					continue;

				update_entity_data(&players[cp], entBase);

				cp++;
			}

			int cf = 0, ce = 0;

			for (int i = 0; i < cp; i++) {
				if (players[i].team == me.team) {
					entFriendlies[cf] = players[i];
					cf++;
				}
				else {
					entEnemies[ce] = players[i];
					ce++;
				}
			}
			iEnemies = ce;
			iFriendlies = cf;
		}
	}
};


class virtualesp
{
private:
	static void glow_player(DWORD mObj, float r, float g, float b)
	{
		memory.Write<float>(mObj + 0x4, r);
		memory.Write<float>(mObj + 0x8, g);
		memory.Write<float>(mObj + 0xC, b);
		memory.Write<float>(mObj + 0x10, 1.0f);
		memory.Write<BOOL>(mObj + 0x24, true);
		memory.Write<BOOL>(mObj + 0x25, false);
	}

	static float SanitizeColor(int value)
	{
		if (value > 255) value = 255;
		if (value < 0) value = 0;
		return (float)value / 255;
	}
public:
	static void start_engine() {
		while (!memory.Attach("csgo.exe", PROCESS_ALL_ACCESS)) {
			Sleep(100);
		}
		do {
			Sleep(1000);
			offset::get_offset(&memory);
		} while (dwLocalPlayer < 65535);
		CreateThread(NULL, NULL, &offset::scan_offsets, NULL, NULL, NULL);
	}

	static unsigned long __stdcall esp_thread(void*)
	{
		int objectCount;
		DWORD pointerToGlow;
		Entity* Player = NULL;
		float Friend = SanitizeColor(100);
		float Enemy = SanitizeColor(140);

		while (true)
		{
			pointerToGlow = memory.Read<DWORD>(GetClientModule()->dwBase + dwGlow);
			objectCount = memory.Read<DWORD>(GetClientModule()->dwBase + dwGlow + 0x4);
			if (pointerToGlow != NULL && objectCount > 0)
			{
				for (int i = 0; i < objectCount; i++)
				{
					DWORD mObj = pointerToGlow + i * sizeof(glow_t);
					glow_t glowObject = memory.Read<glow_t>(mObj);
					Player = GetEntityByBase(glowObject.dwBase);

					if (glowObject.dwBase == NULL || Player == nullptr || Player->is_dormant) {
						continue;
					}
					if (me.team == Player->team) {
						glow_player(mObj, 0, 0, Friend);
					}
					else {
						glow_player(mObj, Enemy, 0, 0);
					}
				}
			}
		}
		return EXIT_SUCCESS;
	}
};


int main() 
{
	bool enabled = false;
	HANDLE ESP = NULL;

	virtualesp::start_engine();
	std::cout << "F1 to toggle ESP!" << std::endl;
	while (TRUE)
	{
		if (GetAsyncKeyState(VK_F1) & 1) {
			enabled = !enabled;
			if (enabled) {
				std::cout << "ESP: on" << std::endl;
				ESP = CreateThread(NULL, NULL, &virtualesp::esp_thread, NULL, NULL, NULL);
			}
			else {
				std::cout << "ESP: off" << std::endl;
				TerminateThread(ESP, 0);
				CloseHandle(ESP);
			}
		}
	}
}