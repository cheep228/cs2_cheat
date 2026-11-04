// ============================================================================
//  NoomiCloneX — плавающее мод-меню 🎃 (Frida + frida-il2cpp-bridge 0.14)
//  Движок: Unity 6 (6000.6) IL2CPP, arm64. Инъекция в рантайме, APK не трогаем.
//
//  Возможности:
//    • перетаскиваемая кнопка-тыква поверх игры (без разрешения overlay);
//    • установка денег и "перехватов" (числовые поля);
//    • Fly / анти-гравитация (toggle);
//    • кнопка "Dump C#" и поиск поля по имени — для донастройки.
//
//  Точные поля денег/перехватов берутся из dump.js. Заполни CONFIG ниже.
//  Пока CONFIG пуст — кнопки работают ЭВРИСТИЧЕСКИ (меняют все числовые поля,
//  чьё имя похоже на money/coin/... и логируют каждое — так найдём настоящее).
// ============================================================================
import "frida-il2cpp-bridge";
import Java from "frida-java-bridge"; // в Frida 17 глобального Java нет — импортируем

// ============================================================================
//  АНТИ-KILL ПРЕАМБУЛА — выполняется СРАЗУ при загрузке gadget, до NXGATE.
//  NXGATE (libge32…so) при старте детектит frida и делает тихий exit/kill.
//  Мы перехватываем пути выхода, чтобы он не смог убить процесс.
// ============================================================================
const AK_LOG = "/data/data/com.JamesWest.NoomiClone/.cache/menu.log";
function akLog(s: string): void {
  const line = "[AK] " + s;
  try { console.log(line); } catch (_) {}
  try { const f = new File(AK_LOG, "a"); f.write(line + "\n"); f.flush(); f.close(); } catch (_) {}
}
function akExport(mod: string, name: string): NativePointer | null {
  try { return Process.getModuleByName(mod).getExportByName(name); } catch { return null; }
}
// Смертельные сигналы, которые реально убивают/роняют процесс.
// ВАЖНО: НЕ трогаем служебные сигналы ART (SIGQUIT=3, SIGURG=23, RT 34+),
// иначе ломается рантайм и игра виснет чёрным экраном.
const LETHAL = new Set<number>([4 /*ILL*/, 6 /*ABRT*/, 7 /*BUS*/, 8 /*FPE*/, 9 /*KILL*/, 11 /*SEGV*/, 31 /*SYS*/]);

(function antiKill(): void {
  try {
    akLog("preamble start, pid=" + Process.id);
    const myPid = Process.id;

    // 1) exit / _exit / _Exit / quick_exit / abort -> no-op (вернуть управление)
    const seen = new Set<string>();
    const voidExit: [string, string, string[]][] = [
      ["exit", "void", ["int"]],
      ["_exit", "void", ["int"]],
      ["_Exit", "void", ["int"]],
      ["quick_exit", "void", ["int"]],
      ["abort", "void", []],
    ];
    for (const [name, ret, args] of voidExit) {
      const p = akExport("libc.so", name);
      if (!p) continue;
      const key = p.toString();
      if (seen.has(key)) continue;
      seen.add(key);
      try {
        Interceptor.replace(p, new NativeCallback(function () { akLog("blocked " + name); }, ret as any, args as any));
      } catch (e) { akLog("replace " + name + " fail: " + e); }
    }

    // 2) kill / tgkill / pthread_kill / raise -> глушим ТОЛЬКО смертельные сигналы в себя
    const kill = akExport("libc.so", "kill");
    if (kill) Interceptor.attach(kill, { onEnter(a) { const pid = a[0].toInt32(); const sig = a[1].toInt32(); if ((pid === myPid || pid === 0 || pid === -1) && LETHAL.has(sig)) { akLog("block kill sig " + sig); a[1] = ptr(0); } } });
    const tgkill = akExport("libc.so", "tgkill");
    if (tgkill) Interceptor.attach(tgkill, { onEnter(a) { const tgid = a[0].toInt32(); const sig = a[2].toInt32(); if (tgid === myPid && LETHAL.has(sig)) { akLog("block tgkill sig " + sig); a[2] = ptr(0); } } });
    const pthread_kill = akExport("libc.so", "pthread_kill");
    if (pthread_kill) Interceptor.attach(pthread_kill, { onEnter(a) { const sig = a[1].toInt32(); if (LETHAL.has(sig)) { akLog("block pthread_kill sig " + sig); a[1] = ptr(0); } } });
    const raiseP = akExport("libc.so", "raise");
    if (raiseP) {
      try {
        const origRaise = new NativeFunction(raiseP, "int", ["int"]);
        Interceptor.replace(raiseP, new NativeCallback(function (sig: number) {
          if (LETHAL.has(sig)) { akLog("block raise sig " + sig); return 0; }
          return origRaise(sig);
        }, "int", ["int"]));
      } catch (_) {}
    }

    // 3) Java-пути выхода
    try {
      Java.perform(() => {
        try { const S = Java.use("java.lang.System"); S.exit.implementation = function (c: number) { akLog("blocked System.exit " + c); }; } catch (_) {}
        try { const R = Java.use("java.lang.Runtime"); R.exit.implementation = function (c: number) { akLog("blocked Runtime.exit"); }; R.halt.implementation = function (c: number) { akLog("blocked Runtime.halt"); }; } catch (_) {}
        try {
          const P = Java.use("android.os.Process");
          const myJavaPid = P.myPid();
          P.killProcess.implementation = function (pid: number) { if (pid === myJavaPid) { akLog("blocked killProcess self"); return; } return this.killProcess(pid); };
        } catch (_) {}
      });
    } catch (e) { akLog("java hooks fail: " + e); }

    akLog("preamble installed OK");
  } catch (e) { akLog("preamble fatal: " + e); }
})();

// ===== АНТИ-ДЕТЕКТ: прячем следы из /proc (нативно) =====
// NXGATE сканит /proc/self/maps (ищет наш gadget) и /proc/self/task/*/comm
// (ищет имена потоков frida). Делаем:
//  1) openat/open на /proc/self/maps -> подсовываем «чистую» копию без наших строк;
//  2) переименовываем потоки frida в безобидные (нативным write в comm).
const keepAlive: any[] = []; // держим ссылки на строки, чтобы GC их не собрал
const MAPS_BAD = /libsecmon|\.cache\/lib|frida|gum|gadget|linjector|memfd:frida|\/libsec\//i;
const COMM_BAD = /gum|frida|gdbus|gmain|linjector|gadget|pool-frida|gjs/i;

(function procEvasion(): void {
  try {
    const myPid = Process.id;
    const pOpen = akExport("libc.so", "open");
    const pOpenat = akExport("libc.so", "openat");
    const pRead = akExport("libc.so", "read");
    const pClose = akExport("libc.so", "close");
    const pWrite = akExport("libc.so", "write");
    if (!pOpen || !pRead || !pClose || !pWrite) { akLog("proc evasion: no libc fns"); return; }
    const fnOpen = new NativeFunction(pOpen, "int", ["pointer", "int", "int"]);
    const fnRead = new NativeFunction(pRead, "long", ["int", "pointer", "ulong"]);
    const fnClose = new NativeFunction(pClose, "int", ["int"]);
    const fnWrite = new NativeFunction(pWrite, "long", ["int", "pointer", "ulong"]);
    const toNum = (v: any): number => (typeof v === "number" ? v : (v && v.toNumber ? v.toNumber() : Number(v)));

    function readAll(path: string): string | null {
      try {
        const pp = Memory.allocUtf8String(path);
        const fd = toNum(fnOpen(pp, 0, 0)); // O_RDONLY
        if (fd < 0) return null;
        const sz = 65536;
        const buf = Memory.alloc(sz);
        let out = "";
        let guard = 0;
        while (guard++ < 2048) {
          const n = toNum(fnRead(fd, buf, sz));
          if (n <= 0) break;
          out += buf.readUtf8String(n);
        }
        fnClose(fd);
        return out;
      } catch (_) { return null; }
    }

    let ctr = 0;
    function writeCleanMaps(): string | null {
      const data = readAll("/proc/self/maps");
      if (!data) return null;
      const clean = data.split("\n").filter((l) => !MAPS_BAD.test(l)).join("\n");
      try {
        const path = "/data/data/com.JamesWest.NoomiClone/.cache/mp" + (ctr++ % 4);
        const pp = Memory.allocUtf8String(path);
        const fd = toNum(fnOpen(pp, 0x241, 0x1a4)); // O_WRONLY|O_CREAT|O_TRUNC, 0644
        if (fd < 0) return null;
        const cb = Memory.allocUtf8String(clean);
        fnWrite(fd, cb, clean.length);
        fnClose(fd);
        return path;
      } catch (_) { return null; }
    }

    function isMapsPath(p: string): boolean {
      return p === "/proc/self/maps" || p === "/proc/" + myPid + "/maps" ||
        /^\/proc\/self\/task\/\d+\/maps$/.test(p) || /^\/proc\/\d+\/maps$/.test(p);
    }

    // openat(dirfd, path, flags, mode): path = args[1]
    if (pOpenat) {
      Interceptor.attach(pOpenat, {
        onEnter(args) {
          try {
            const p = args[1].readCString();
            if (p && isMapsPath(p)) {
              const clean = writeCleanMaps();
              if (clean) { const s = Memory.allocUtf8String(clean); keepAlive.push(s); args[1] = s; akLog("maps->clean (openat)"); }
            }
          } catch (_) {}
        },
      });
    }
    // open(path, flags, mode): path = args[0]
    Interceptor.attach(pOpen, {
      onEnter(args) {
        try {
          const p = args[0].readCString();
          if (p && isMapsPath(p)) {
            const clean = writeCleanMaps();
            if (clean) { const s = Memory.allocUtf8String(clean); keepAlive.push(s); args[0] = s; akLog("maps->clean (open)"); }
          }
        } catch (_) {}
      },
    });

    // переименование потоков frida в comm
    function renameThreads(): void {
      try {
        for (const t of Process.enumerateThreads()) {
          const cp = "/proc/self/task/" + t.id + "/comm";
          const nm = readAll(cp);
          if (nm && COMM_BAD.test(nm)) {
            try {
              const pp = Memory.allocUtf8String(cp);
              const fd = toNum(fnOpen(pp, 1, 0)); // O_WRONLY
              if (fd >= 0) {
                const nn = "Thread-" + (t.id % 100000);
                const nb = Memory.allocUtf8String(nn);
                fnWrite(fd, nb, nn.length);
                fnClose(fd);
                akLog("rename tid " + t.id);
              }
            } catch (_) {}
          }
        }
      } catch (_) {}
    }
    renameThreads();
    let k = 0;
    const iv = setInterval(() => { renameThreads(); if (++k > 80) clearInterval(iv); }, 300);

    akLog("proc evasion installed");
  } catch (e) { akLog("proc evasion fatal: " + e); }
})();

// ------------------------- НАСТРОЙКА ПОД ИГРУ -------------------------------
interface FieldSpec { image: string; klass: string; field: string; static?: boolean; }
const CONFIG: { money?: FieldSpec; intercept?: FieldSpec } = {
  // money:     { image: "Assembly-CSharp.dll", klass: "GameData", field: "coins", static: true },
  // intercept: { image: "Assembly-CSharp.dll", klass: "Player",   field: "grips", static: false },
};

const RE_MONEY = /money|coin|cash|gold|gem|crystal|currency|balance|wallet|credit|monet|zolot|den[gь]/i;
const RE_COUNT = /intercept|perehvat|perexvat|grip|regrip|combo|reps|pullup|turnik/i;
const NUMERIC = new Set([
  "System.Int32", "System.Int64", "System.Single", "System.Double",
  "System.UInt32", "System.UInt64", "System.Int16", "System.UInt16", "System.Byte",
]);
const SKIP_ASM = /^(mscorlib|System|Unity|netstandard|Mono\.|Microsoft|I18N|Newtonsoft)/i;
const DUMP_PATH = "/data/data/com.JamesWest.NoomiClone/noomi_dump.cs";
// ----------------------------------------------------------------------------

let flyOn = false;
let flyTimer: any = null;
let toast: (s: string) => void = (s) => console.log("[toast]", s);
const log = (s: string) => console.log("[menu]", s);

// ===================== IL2CPP хелперы =====================
function img(name: string): Il2Cpp.Image | null {
  try { const a = Il2Cpp.domain.tryAssembly(name); return a ? a.image : null; } catch { return null; }
}
function cls(image: string, klass: string): Il2Cpp.Class | null {
  const i = img(image); return i ? i.tryClass(klass) : null;
}
function coerce(typeName: string, raw: string): any {
  const n = Number(raw);
  if (typeName === "System.Int64" || typeName === "System.UInt64") return int64(raw);
  if (typeName === "System.Boolean") return !!n;
  return n;
}
function gameImages(): Il2Cpp.Image[] {
  const out: Il2Cpp.Image[] = [];
  for (const asm of Il2Cpp.domain.assemblies) {
    try { const i = asm.image; if (!SKIP_ASM.test(i.name)) out.push(i); } catch (_) {}
  }
  return out;
}

function setBySpec(spec: FieldSpec, raw: string): number {
  const klass = cls(spec.image, spec.klass);
  if (!klass) throw new Error(`класс не найден: ${spec.image}/${spec.klass}`);
  const field: any = klass.field(spec.field);
  const tn = field.type.name as string;
  const val = coerce(tn, raw);
  let changed = 0;
  if (spec.static || field.isStatic) {
    klass.initialize();
    field.value = val; changed++;
    log(`static ${spec.klass}.${spec.field} = ${raw}`);
  } else {
    for (const obj of Il2Cpp.gc.choose(klass)) {
      try { (obj as any).field(spec.field).value = val; changed++; } catch (_) {}
    }
    log(`inst ${spec.klass}.${spec.field} = ${raw} (объектов: ${changed})`);
  }
  return changed;
}

function setByHeuristic(re: RegExp, raw: string): number {
  let changed = 0, touched = 0;
  for (const image of gameImages()) {
    for (const klass of image.classes) {
      let fields: Il2Cpp.Field[];
      try { fields = klass.fields; } catch { continue; }
      for (const f of fields) {
        if (!re.test(f.name)) continue;
        let tn = ""; try { tn = f.type.name; } catch { continue; }
        if (!NUMERIC.has(tn)) continue;
        const val = coerce(tn, raw);
        try {
          if (f.isStatic) {
            klass.initialize();
            (f as any).value = val; changed++;
            log(`[эвр] static ${klass.type.name}.${f.name} := ${raw}`);
          } else {
            for (const obj of Il2Cpp.gc.choose(klass)) {
              try { (obj as any).field(f.name).value = val; changed++; } catch (_) {}
            }
            log(`[эвр] inst ${klass.type.name}.${f.name} := ${raw}`);
          }
          if (++touched > 60) { log("[эвр] обрезано (>60)"); return changed; }
        } catch (_) {}
      }
    }
  }
  return changed;
}

function setMoney(raw: string): void {
  try { const n = CONFIG.money ? setBySpec(CONFIG.money, raw) : setByHeuristic(RE_MONEY, raw); toast(`Деньги: полей ${n}`); }
  catch (e) { toast("Ошибка денег: " + e); }
}
function setIntercept(raw: string): void {
  try { const n = CONFIG.intercept ? setBySpec(CONFIG.intercept, raw) : setByHeuristic(RE_COUNT, raw); toast(`Перехваты: полей ${n}`); }
  catch (e) { toast("Ошибка перехватов: " + e); }
}

// Fly = обнуление гравитации («парение», без падений). Направленный полёт
// допилим, когда увидим класс контроллера персонажа в дампе.
function applyGravity(y: number): void {
  try {
    const Physics = cls("UnityEngine.PhysicsModule.dll", "UnityEngine.Physics");
    if (!Physics) return;
    const g: any = Physics.method("get_gravity").invoke();
    g.field("y").value = y;
    Physics.method("set_gravity").invoke(g);
  } catch (e) { log("gravity: " + e); }
}
function rigidbodiesGravity(enabled: boolean): number {
  let n = 0;
  const Rb = cls("UnityEngine.PhysicsModule.dll", "UnityEngine.Rigidbody");
  if (!Rb) return 0;
  for (const obj of Il2Cpp.gc.choose(Rb)) {
    try { (obj as any).method("set_useGravity").invoke(enabled); n++; } catch (_) {}
  }
  return n;
}
function toggleFly(): void {
  flyOn = !flyOn;
  if (flyOn) {
    applyGravity(0);
    const n = rigidbodiesGravity(false);
    flyTimer = setInterval(() => applyGravity(0), 1500);
    toast(`Fly ON (rb: ${n})`);
  } else {
    if (flyTimer) { clearInterval(flyTimer); flyTimer = null; }
    applyGravity(-9.81);
    rigidbodiesGravity(true);
    toast("Fly OFF");
  }
}

function writeFullDump(path: string): void {
  let file: any;
  try { file = new File(path, "w"); } catch (e) { toast("дамп: не открыть файл"); return; }
  for (const asm of Il2Cpp.domain.assemblies) {
    let image: Il2Cpp.Image; try { image = asm.image; } catch { continue; }
    file.write(`\n// ===== ${image.name} =====\n`);
    for (const klass of image.classes) {
      let fields: Il2Cpp.Field[], methods: Il2Cpp.Method[];
      try { fields = klass.fields; methods = klass.methods; } catch { continue; }
      file.write(`class ${klass.type.name} {\n`);
      for (const f of fields) { try { file.write(`  ${f.isStatic ? "static " : ""}${f.type.name} ${f.name}; // 0x${f.offset.toString(16)}\n`); } catch (_) {} }
      for (const m of methods) { try { file.write(`  ${m.isStatic ? "static " : ""}${m.returnType.name} ${m.name}(${m.parameterCount});\n`); } catch (_) {} }
      file.write(`}\n`);
    }
  }
  try { file.flush(); file.close(); } catch (_) {}
}
function dumpCs(): void { writeFullDump(DUMP_PATH); toast("Дамп: " + DUMP_PATH); }

function searchField(q: string): void {
  if (!q) return;
  const re = new RegExp(q, "i");
  let hits = 0;
  for (const image of gameImages()) {
    for (const klass of image.classes) {
      let fields: Il2Cpp.Field[]; try { fields = klass.fields; } catch { continue; }
      for (const f of fields) {
        if (!re.test(f.name)) continue;
        let tn = "?"; try { tn = f.type.name; } catch {}
        console.log(`  ${f.isStatic ? "S" : "i"} ${image.name} | ${klass.type.name}.${f.name} : ${tn}`);
        if (++hits > 200) { console.log("  ...обрезано"); toast("Найдено >200, см. консоль"); return; }
      }
    }
  }
  toast(`Поиск "${q}": ${hits} (см. консоль)`);
}

// выполнить il2cpp-операцию с гарантированным attach
function perform(fn: () => void): void {
  try { Il2Cpp.perform(fn); } catch (e) { console.log("[il2cpp]", e); }
}

// ============================== UI (Java) ==================================
function buildUI(): void {
  const ActivityThread = Java.use("android.app.ActivityThread");
  const ActivityClientRecord = Java.use("android.app.ActivityThread$ActivityClientRecord");
  const FrameLayout = Java.use("android.widget.FrameLayout");
  const LinearLayout = Java.use("android.widget.LinearLayout");
  const TextView = Java.use("android.widget.TextView");
  const EditText = Java.use("android.widget.EditText");
  const Button = Java.use("android.widget.Button");
  const Toast = Java.use("android.widget.Toast");
  const Color = Java.use("android.graphics.Color");
  const Gravity = Java.use("android.view.Gravity");
  const View = Java.use("android.view.View");
  const ViewGroupLP = Java.use("android.view.ViewGroup$LayoutParams");
  const FrameLP = Java.use("android.widget.FrameLayout$LayoutParams");
  const LinearLP = Java.use("android.widget.LinearLayout$LayoutParams");
  const InputType = Java.use("android.text.InputType");
  const MotionEvent = Java.use("android.view.MotionEvent");
  const TypedValue = Java.use("android.util.TypedValue");
  const OnClickListener = Java.use("android.view.View$OnClickListener");
  const OnTouchListener = Java.use("android.view.View$OnTouchListener");

  function getTopActivity(): any {
    const at = ActivityThread.currentActivityThread();
    const map = at.mActivities.value;
    const vals = map.values().toArray();
    for (const r of vals) {
      const rec = Java.cast(r, ActivityClientRecord);
      if (!rec.paused.value) return rec.activity.value;
    }
    if (vals.length) return Java.cast(vals[0], ActivityClientRecord).activity.value;
    return null;
  }

  const activity = getTopActivity();
  if (!activity) throw new Error("activity not ready");
  const ctx = activity;

  const dp = (v: number): number =>
    Math.round(TypedValue.applyDimension(TypedValue.COMPLEX_UNIT_DIP.value, v, ctx.getResources().getDisplayMetrics()));

  toast = (s: string) => {
    Java.scheduleOnMainThread(() => { try { Toast.makeText(ctx, "🎃 " + s, Toast.LENGTH_SHORT.value).show(); } catch (_) {} });
    console.log("[toast]", s);
  };

  const uid = () => Math.floor(Math.random() * 1e7);
  function onClick(fn: () => void): any {
    const L = Java.registerClass({
      name: "noomi.C" + uid(),
      implements: [OnClickListener],
      methods: { onClick(_v: any) { perform(fn); } },
    });
    return L.$new();
  }

  // --- панель ---
  const panel = LinearLayout.$new(ctx);
  panel.setOrientation(LinearLayout.VERTICAL.value);
  panel.setBackgroundColor(Color.parseColor("#E6241a1a"));
  panel.setPadding(dp(12), dp(12), dp(12), dp(12));
  panel.setVisibility(View.GONE.value);

  const title = TextView.$new(ctx);
  title.setText("NoomiCloneX  🎃");
  title.setTextColor(Color.parseColor("#FFB347"));
  title.setTextSize(18.0);
  panel.addView(title);

  function numRow(label: string, hint: string, onSet: (v: string) => void): void {
    const row = LinearLayout.$new(ctx);
    row.setOrientation(LinearLayout.HORIZONTAL.value);
    const et = EditText.$new(ctx);
    et.setHint(hint);
    et.setTextColor(Color.WHITE.value);
    et.setInputType(InputType.TYPE_CLASS_NUMBER.value);
    et.setLayoutParams(LinearLP.$new(0, ViewGroupLP.WRAP_CONTENT.value, 1.0));
    const b = Button.$new(ctx);
    b.setText(label);
    b.setOnClickListener(onClick(() => { const t = et.getText().toString(); onSet(t && t.length ? t : "0"); }));
    row.addView(et); row.addView(b);
    panel.addView(row);
  }

  numRow("Деньги", "сумма", setMoney);
  numRow("Перехваты", "кол-во", setIntercept);

  const flyBtn = Button.$new(ctx);
  flyBtn.setText("Fly: OFF");
  flyBtn.setOnClickListener(onClick(() => { toggleFly(); Java.scheduleOnMainThread(() => flyBtn.setText(flyOn ? "Fly: ON" : "Fly: OFF")); }));
  panel.addView(flyBtn);

  // поиск поля
  const sRow = LinearLayout.$new(ctx);
  sRow.setOrientation(LinearLayout.HORIZONTAL.value);
  const sEt = EditText.$new(ctx);
  sEt.setHint("искать поле");
  sEt.setTextColor(Color.WHITE.value);
  sEt.setLayoutParams(LinearLP.$new(0, ViewGroupLP.WRAP_CONTENT.value, 1.0));
  const sBtn = Button.$new(ctx);
  sBtn.setText("Поиск");
  sBtn.setOnClickListener(onClick(() => searchField(sEt.getText().toString())));
  sRow.addView(sEt); sRow.addView(sBtn);
  panel.addView(sRow);

  const dBtn = Button.$new(ctx);
  dBtn.setText("Dump C#");
  dBtn.setOnClickListener(onClick(dumpCs));
  panel.addView(dBtn);

  // --- тыква ---
  const pumpkin = TextView.$new(ctx);
  pumpkin.setText("🎃");
  pumpkin.setTextSize(28.0);
  pumpkin.setBackgroundColor(Color.parseColor("#AA000000"));
  pumpkin.setPadding(dp(10), dp(6), dp(10), dp(6));

  const container = LinearLayout.$new(ctx);
  container.setOrientation(LinearLayout.VERTICAL.value);
  container.addView(pumpkin);
  container.addView(panel);

  const ts = { x0: 0, y0: 0, tx: 0, ty: 0, moved: false };
  const touchL = Java.registerClass({
    name: "noomi.T" + uid(),
    implements: [OnTouchListener],
    methods: {
      onTouch(_v: any, ev: any) {
        const a = ev.getActionMasked();
        if (a === MotionEvent.ACTION_DOWN.value) {
          ts.x0 = ev.getRawX(); ts.y0 = ev.getRawY();
          ts.tx = container.getTranslationX(); ts.ty = container.getTranslationY();
          ts.moved = false; return true;
        } else if (a === MotionEvent.ACTION_MOVE.value) {
          const dx = ev.getRawX() - ts.x0, dy = ev.getRawY() - ts.y0;
          if (Math.abs(dx) > 10 || Math.abs(dy) > 10) ts.moved = true;
          container.setTranslationX(ts.tx + dx);
          container.setTranslationY(ts.ty + dy);
          return true;
        } else if (a === MotionEvent.ACTION_UP.value) {
          if (!ts.moved) {
            const vis = panel.getVisibility();
            panel.setVisibility(vis === View.GONE.value ? View.VISIBLE.value : View.GONE.value);
          }
          return true;
        }
        return false;
      },
    },
  });
  pumpkin.setOnTouchListener(touchL.$new());

  // крепим в decorView окна игры (без разрешения overlay)
  const decor = activity.getWindow().getDecorView();
  const content = Java.cast(decor, FrameLayout);
  const clp = FrameLP.$new(ViewGroupLP.WRAP_CONTENT.value, ViewGroupLP.WRAP_CONTENT.value);
  clp.gravity.value = (Gravity.TOP.value | Gravity.START.value);
  clp.leftMargin.value = dp(8);
  clp.topMargin.value = dp(80);
  content.addView(container, clp);

  console.log("[+] Меню-тыква прикреплено.");
  toast("меню готово");
}

// ============================== ЗАПУСК ====================================
// UI строим строго на главном потоке Android (иначе CalledFromWrongThread).
let built = false;

function tryBuild(tries: number): void {
  if (built) return;
  Java.perform(() => {
    Java.scheduleOnMainThread(() => {
      if (built) return;
      try { buildUI(); built = true; }
      catch (e) { if (tries % 4 === 0) console.log("[..] жду активити игры..."); }
    });
  });
  if (!built && tries < 120) setTimeout(() => tryBuild(tries + 1), 1500);
}

Il2Cpp.perform(() => {
  console.log("[*] il2cpp готов. Unity", Il2Cpp.unityVersion);
  tryBuild(0);
});
