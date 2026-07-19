# SharpEmu Android — Plano de Emulação Real

## Análise da Arquitetura Atual (Desktop)

### Como o SharpEmu executa jogos PS4/PS5:

```
┌─────────────────────────────────────────────────────────────┐
│                    SharpEmu Desktop                          │
├─────────────────────────────────────────────────────────────┤
│  GUI (Avalonia) → CLI → Runtime → CpuDispatcher             │
│                                        │                     │
│                              DirectExecutionBackend          │
│                              (executa x86-64 NATIVO)        │
│                                        │                     │
│  ┌─────────────┐  ┌──────────────┐  ┌──────────────────┐   │
│  │ SELF Loader │  │ HLE Modules  │  │ Vulkan/Metal GPU │   │
│  │ (eboot.bin) │  │ (libSce*)    │  │ (AGC → SPIR-V)  │   │
│  └─────────────┘  └──────────────┘  └──────────────────┘   │
└─────────────────────────────────────────────────────────────┘
```

**Ponto crítico**: O `DirectExecutionBackend` NÃO é um interpretador nem um JIT
recompiler. Ele executa as instruções x86-64 do jogo DIRETAMENTE na CPU host.
Isso só funciona porque PS4/PS5 usam x86-64 (AMD Jaguar/Zen 2) e o host também é x86-64.

### Componentes reutilizáveis no Android (100%):
- ✅ HLE layer (todas as implementações libSce* — Kernel, VideoOut, Gpu, Pad, Audio, etc.)
- ✅ Shader Compiler (AGC → SPIR-V) — já gera SPIR-V puro
- ✅ Vulkan GPU Backend — Android tem Vulkan nativo
- ✅ SELF/ELF Loader — parsing de binários PS4/PS5
- ✅ Module Manager / NID dispatch — resolução de imports
- ✅ File System abstraction — mapeamento de /app0, /save0, etc.
- ✅ Aerolib catalog — catálogo de símbolos

### Componentes que NÃO funcionam em ARM:
- ❌ `DirectExecutionBackend` — executa x86-64 nativamente (impossível em ARM)
- ❌ `JitStubs` — emite código x86-64 em runtime
- ❌ `CpuPatcher` — patcheia instruções x86-64
- ❌ Signal handlers (VEH/sigaction) — específicos para faults x86-64
- ❌ `PhysicalVirtualMemory` (parcialmente) — usa mmap mas assume endereços x86-64

---

## Estratégia: FEX-EMU como Backend de CPU

### Por que FEX-EMU?

FEX-EMU é um emulador x86/x86-64 → ARM64 com JIT recompilation de alta performance.
É a mesma tecnologia usada pelo Winlator, Termux-x11, e projetos similares.

**Vantagens:**
- JIT maduro e otimizado (x86-64 → ARM64)
- Suporta SSE/SSE2/SSE3/SSSE3/SSE4.1/SSE4.2/AVX (parcial)
- Threaded — múltiplas threads guest rodam em paralelo
- Escrito em C++ compilável para Android/ARM64 via NDK
- Licença MIT — compatível com GPL do SharpEmu
- Ativamente mantido e usado em produção

### Arquitetura Proposta para Android:

```
┌─────────────────────────────────────────────────────────────────┐
│                    SharpEmu Android                              │
├─────────────────────────────────────────────────────────────────┤
│                                                                  │
│  ┌──────────────────────────────────────────────────────────┐   │
│  │  Android UI (Kotlin/Compose — Material You 3 Expressive) │   │
│  │  Library / Settings / Launch / Console                    │   │
│  └──────────────────────────┬───────────────────────────────┘   │
│                             │ JNI / Process                      │
│  ┌──────────────────────────▼───────────────────────────────┐   │
│  │  .NET Runtime (Android) OU Native C++ Bridge             │   │
│  │  SharpEmu.Core + HLE + Libs (reutilizado do desktop)     │   │
│  └──────────────────────────┬───────────────────────────────┘   │
│                             │                                    │
│  ┌──────────────────────────▼───────────────────────────────┐   │
│  │  FexExecutionBackend : INativeCpuBackend                  │   │
│  │  ┌─────────────────────────────────────────────────────┐  │   │
│  │  │  FEX-EMU Core (C++ / ARM64 JIT)                    │  │   │
│  │  │  - x86-64 → ARM64 translation                      │  │   │
│  │  │  - Syscall interception → HLE dispatch             │  │   │
│  │  │  - Memory mapping (guest address space)            │  │   │
│  │  │  - Thread management                               │  │   │
│  │  └─────────────────────────────────────────────────────┘  │   │
│  └──────────────────────────┬───────────────────────────────┘   │
│                             │                                    │
│  ┌──────────────────────────▼───────────────────────────────┐   │
│  │  Vulkan Renderer (Android)                                │   │
│  │  - SPIR-V shaders (do ShaderCompiler existente)          │   │
│  │  - ANativeWindow surface                                  │   │
│  │  - Texture cache / framebuffer management                 │   │
│  └──────────────────────────────────────────────────────────┘   │
│                                                                  │
│  ┌──────────────────────────────────────────────────────────┐   │
│  │  Android Services                                         │   │
│  │  - AAudio (saída de áudio)                                │   │
│  │  - Input (gamepad/touch → libScePad HLE)                 │   │
│  │  - Storage Access Framework (jogos)                       │   │
│  │  - Notifications (status de emulação)                     │   │
│  └──────────────────────────────────────────────────────────┘   │
└─────────────────────────────────────────────────────────────────┘
```

---

## Plano de Implementação em Fases

### Fase 1: Fundação (2-3 semanas)
**Objetivo**: FEX-EMU compilado e rodando no Android, executando um binário x86-64 simples.

1. Compilar FEX-EMU para ARM64 Android via NDK
   - Clonar FEX-EMU, adaptar CMakeLists para Android NDK
   - Resolver dependências (FEX usa fmt, xxhash, robin-map)
   - Target: API 28+ (Android 9), arm64-v8a

2. Criar bridge JNI (Kotlin ↔ C++ ↔ FEX)
   - Interface mínima: init, loadBinary, execute, stop
   - Callback para interceptar syscalls/imports

3. Teste de prova de conceito
   - Compilar um "Hello World" x86-64 estático
   - Executar via FEX no Android
   - Verificar output correto

### Fase 2: Integração com SharpEmu (3-4 semanas)
**Objetivo**: Substituir DirectExecutionBackend por FexExecutionBackend.

1. Implementar `FexExecutionBackend : INativeCpuBackend`
   ```csharp
   public interface INativeCpuBackend {
       bool TryExecute(CpuContext context, ulong entryPoint, ...);
   }
   ```
   - Mapear CpuContext → FEX CPU state
   - Mapear importStubs → FEX syscall hooks → HLE dispatch
   - Mapear memory regions → FEX guest memory

2. Adaptar PhysicalVirtualMemory para Android
   - Android limita mmap em endereços altos (ASLR)
   - Usar FEX's internal memory manager ou mmap com MAP_FIXED em range baixo
   - GuestAllocationArena precisa caber no address space do Android (39-bit VA)

3. Conectar SELF Loader → FEX
   - Loader parseia eboot.bin (reutilizado 100%)
   - Mapeia segmentos ELF na memória guest do FEX
   - Entry point passado para FEX iniciar execução

### Fase 3: GPU e Rendering (4-6 semanas)
**Objetivo**: Renderizar gráficos via Vulkan no Android.

1. Adaptar VulkanVideoPresenter para Android
   - Substituir HWND/X11 surface por ANativeWindow
   - Usar VkAndroidSurfaceKHR
   - Adaptar swapchain creation para Android

2. Shader Compiler já funciona (AGC → SPIR-V)
   - SPIR-V é universal — funciona em qualquer Vulkan
   - Possível necessidade de adaptar para Vulkan mobile (subset)
   - Verificar suporte a features: geometry shader, tessellation

3. Texture/Framebuffer management
   - Adaptar para GPUs mobile (Adreno, Mali, PowerVR)
   - Considerar limitações de bandwidth (tile-based rendering)

### Fase 4: Áudio, Input e Storage (2-3 semanas)
**Objetivo**: Completar a experiência do usuário.

1. Áudio: AAudio backend para IHostAudioOutput
   - Implementar IHostAudioOutput com AAudio/OpenSL ES
   - Resampling se necessário (PS4 usa 48kHz, Android suporta)

2. Input: Mapear touch/gamepad → libScePad HLE
   - Bluetooth gamepads (DualShock 4, DualSense)
   - Touch controls virtuais (on-screen)
   - Implementar HostGamepadState para Android

3. Storage: SAF (Storage Access Framework)
   - Importar jogos via file picker do Android
   - Copiar/gerenciar jogos em app-specific storage
   - Suporte a OBB/expansion files para jogos grandes

### Fase 5: Otimização e Estabilidade (contínuo)
**Objetivo**: Performance jogável.

1. FEX JIT tuning
   - Block cache size optimization para mobile
   - Reduzir overhead de syscall interception
   - Thread affinity para big.LITTLE (Cortex-X + Cortex-A)

2. Memory optimization
   - Android tem limite de ~2-4GB por app
   - Lazy allocation / demand paging
   - Compressed texture cache

3. GPU optimization
   - Shader cache persistente (evitar recompilação)
   - Async pipeline compilation
   - Resolution scaling para mobile

---

## Alternativa: .NET no Android vs Native C++

### Opção A: .NET for Android (recomendado inicialmente)
- Rodar SharpEmu.Core/HLE/Libs como .NET no Android
- FEX-EMU como native library via P/Invoke
- Vantagem: reutiliza 90% do código C# sem reescrita
- Desvantagem: overhead do .NET runtime (~50MB RAM)

### Opção B: Reescrever em C++ nativo
- Portar HLE/Libs para C++
- FEX-EMU integrado diretamente
- Vantagem: menor overhead, mais controle
- Desvantagem: reescrita massiva (6+ meses), perde sincronia com upstream

### Opção C: Híbrido (recomendado a longo prazo)
- .NET para HLE/Libs (lógica de emulação de alto nível)
- C++ nativo para FEX + Vulkan (performance crítica)
- JNI/P-Invoke como bridge
- Melhor equilíbrio entre reutilização e performance

---

## Desafios e Riscos

| Desafio | Severidade | Mitigação |
|---------|-----------|-----------|
| FEX não suporta todas instruções AVX | Alta | Fallback para interpretação (FEX já faz isso) |
| Android limita memória por app (~4GB) | Alta | Lazy alloc, texture streaming, cache eviction |
| GPUs mobile não suportam todas features Vulkan | Média | Feature detection, shader fallbacks |
| Thread scheduling em big.LITTLE | Média | Thread affinity, FEX já tem scheduler |
| SELinux restrictions no Android | Média | Executar dentro do app sandbox |
| Performance insuficiente para jogos pesados | Alta | Focar em jogos 2D/indie primeiro |
| .NET on Android overhead | Baixa | AOT compilation, trimming |

---

## O que podemos aproveitar do FEX-EMU diretamente:

1. **CPU JIT** (x86-64 → ARM64) — o core do FEX
2. **Memory manager** — guest address space management
3. **Syscall layer** — interceptação de syscalls Linux (PS4 usa FreeBSD, mas a interface é similar)
4. **Thread management** — guest thread creation/scheduling
5. **Signal handling** — tradução de faults x86-64 para ARM64

## O que precisamos adaptar:

1. **Syscall ABI**: PS4 usa FreeBSD syscalls, não Linux. O HLE do SharpEmu já intercepta
   via NID (não via syscall number), então o FEX precisa apenas interceptar `int 0x80`/
   `syscall` e redirecionar para o HLE do SharpEmu.

2. **Memory layout**: PS4 espera endereços específicos (0x400000 para eboot, etc.).
   FEX suporta MAP_FIXED, mas o Android ASLR pode conflitar.

3. **TLS (Thread-Local Storage)**: PS4 usa FS/GS segment registers para TLS.
   FEX já emula isso via TPIDR_EL0 no ARM64.

---

## Próximos Passos Imediatos

1. **Compilar FEX-EMU para Android ARM64** (proof of concept)
2. **Criar o FexExecutionBackend** implementando INativeCpuBackend
3. **Testar com um eboot.bin simples** (homebrew PS4)
4. **Iterar** até o primeiro "Hello World" PS4 renderizar na tela

---

## Referências

- FEX-EMU: https://github.com/FEX-Emu/FEX
- Winlator (exemplo de FEX no Android): https://github.com/brunodev85/winlator
- SharpEmu INativeCpuBackend: `src/SharpEmu.Core/Cpu/Native/INativeCpuBackend.cs`
- Vulkan Android: https://developer.android.com/ndk/guides/graphics
- .NET for Android: https://learn.microsoft.com/en-us/dotnet/android/
