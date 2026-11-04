// ============================================================================
//  NoomiCloneX — IL2CPP discovery / dump agent  (frida-il2cpp-bridge 0.14)
//  Запусти один раз на устройстве. Он:
//    1) пишет полный C#-подобный дамп в DUMP_PATH (классы/поля/методы+offset);
//    2) печатает в консоль поля-кандидаты на деньги / перехваты / полёт.
//  По этому дампу прицельно настраивается меню (CONFIG в menu.ts).
// ============================================================================
import "frida-il2cpp-bridge";

// процесс пишет сюда гарантированно (свой uid). Забрать потом рутом:
//   su -c "cp /data/data/com.JamesWest.NoomiClone/noomi_dump.cs /sdcard/"
const DUMP_PATH = "/data/data/com.JamesWest.NoomiClone/noomi_dump.cs";

const MONEY = /money|coin|cash|gold|gem|crystal|dollar|currency|balance|wallet|credit|monet|zolot|den[gь]/i;
const COUNT = /intercept|perehvat|perexvat|grip|regrip|catch|combo|reps|pullup|turnik|level|score/i;
const FLY = /fly|flight|levit|gravity|noclip|velocity|jump|airborne/i;

const NUMERIC = new Set([
  "System.Int32", "System.Int64", "System.Single", "System.Double",
  "System.UInt32", "System.UInt64", "System.Int16", "System.UInt16", "System.Byte", "System.Boolean",
]);
const SKIP = /^(mscorlib|System|Unity|netstandard|Mono\.|Microsoft|I18N|Newtonsoft)/i;

function gameImages(): Il2Cpp.Image[] {
  const out: Il2Cpp.Image[] = [];
  for (const asm of Il2Cpp.domain.assemblies) {
    try {
      const img = asm.image;
      if (!SKIP.test(img.name)) out.push(img);
    } catch (_) {}
  }
  return out;
}

function writeFullDump(path: string): void {
  let file: any;
  try { file = new File(path, "w"); }
  catch (e) { console.log("[!] не могу открыть", path, e); return; }
  let classes = 0;
  for (const asm of Il2Cpp.domain.assemblies) {
    let img: Il2Cpp.Image;
    try { img = asm.image; } catch { continue; }
    file.write(`\n// ===== image: ${img.name} =====\n`);
    for (const klass of img.classes) {
      let fields: Il2Cpp.Field[], methods: Il2Cpp.Method[];
      try { fields = klass.fields; methods = klass.methods; } catch { continue; }
      file.write(`class ${klass.type.name} {\n`);
      for (const f of fields) {
        try {
          file.write(`  ${f.isStatic ? "static " : ""}${f.type.name} ${f.name}; // off 0x${f.offset.toString(16)}\n`);
        } catch (_) {}
      }
      for (const m of methods) {
        try {
          file.write(`  ${m.isStatic ? "static " : ""}${m.returnType.name} ${m.name}(${m.parameterCount});\n`);
        } catch (_) {}
      }
      file.write(`}\n`);
      classes++;
    }
  }
  try { file.flush(); file.close(); } catch (_) {}
  console.log(`[+] Полный дамп записан: ${path}  (классов: ${classes})`);
}

function scan(re: RegExp, onlyNumeric: boolean): void {
  let hits = 0;
  for (const img of gameImages()) {
    for (const klass of img.classes) {
      let fields: Il2Cpp.Field[];
      try { fields = klass.fields; } catch { continue; }
      for (const f of fields) {
        if (!re.test(f.name)) continue;
        let tn = "?"; try { tn = f.type.name; } catch {}
        if (onlyNumeric && !NUMERIC.has(tn)) continue;
        console.log(`  [${f.isStatic ? "static" : "inst  "}] ${img.name} | ${klass.type.name}.${f.name} : ${tn}`);
        if (++hits > 400) { console.log("  ...обрезано (см. полный дамп)"); return; }
      }
    }
  }
  if (!hits) console.log("  — пусто по шаблону");
}

Il2Cpp.perform(() => {
  console.log("[*] Unity:", Il2Cpp.unityVersion);
  console.log("[*] Пишу полный дамп (10-60 сек)...");
  writeFullDump(DUMP_PATH);
  console.log("\n==================== КАНДИДАТЫ: ДЕНЬГИ ====================");
  scan(MONEY, true);
  console.log("\n==================== КАНДИДАТЫ: ПЕРЕХВАТЫ/СЧЁТЧИКИ =======");
  scan(COUNT, true);
  console.log("\n==================== КАНДИДАТЫ: ПОЛЁТ/ФИЗИКА ============");
  scan(FLY, false);
  console.log(`\n[*] Готово. Забери дамп рутом:`);
  console.log(`    su -c "cp ${DUMP_PATH} /sdcard/noomi_dump.cs"`);
});
