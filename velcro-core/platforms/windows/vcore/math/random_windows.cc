#include "random_windows.h"

#include <vcore/platform_incl.h>
#include <Wincrypt.h>

namespace V
{
    BetterPseudoRandom_Windows::BetterPseudoRandom_Windows() {
        if (!CryptAcquireContext((HCRYPTPROV*)&m_generatorHandle, 0, 0, PROV_RSA_FULL, CRYPT_VERIFYCONTEXT | CRYPT_SILENT)) {
            V_Warning("System", false, "CryptAcquireContext failed with 0x%08x\n", GetLastError());
            m_generatorHandle = 0;
        }
    }

    BetterPseudoRandom_Windows::~BetterPseudoRandom_Windows() {
        if (m_generatorHandle) {
            CryptReleaseContext((HCRYPTPROV)m_generatorHandle, 0);
        }
    }

    bool BetterPseudoRandom_Windows::GetRandom(void* data, size_t dataSize) {
        if (!m_generatorHandle) {
            return false;
        }
        if (!CryptGenRandom((HCRYPTPROV)m_generatorHandle, static_cast<DWORD>(dataSize), static_cast<PBYTE>(data))) {
            V_TracePrintf("System", "Failed to call CryptGenRandom!");
            return false;
        }
        return true;
    }
}