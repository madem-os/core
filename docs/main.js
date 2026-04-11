const DISK_IMAGE_URL = "./assets/mademos.img";
const V86_VERSION = "0.5.319";
const V86_BASE_URL = `https://cdn.jsdelivr.net/npm/v86@${V86_VERSION}`;
const V86_GH_REF = "master";
const BIOS_URL = `https://cdn.jsdelivr.net/gh/copy/v86@${V86_GH_REF}/bios/seabios.bin`;
const VGA_BIOS_URL = `https://cdn.jsdelivr.net/gh/copy/v86@${V86_GH_REF}/bios/vgabios.bin`;

const startButton = document.getElementById("start-button");
const resetButton = document.getElementById("reset-button");
const fullscreenButton = document.getElementById("fullscreen-button");
const statusText = document.getElementById("status-text");
const screenContainer = document.getElementById("screen_container");

let emulator = null;

function setStatus(text) {
    statusText.textContent = text;
}

function setControlsRunning(running) {
    startButton.disabled = running;
    resetButton.disabled = !running;
    fullscreenButton.disabled = !running;
}

function bootDemo() {
    setStatus("Booting");
    setControlsRunning(true);

    emulator = new V86({
        wasm_path: `${V86_BASE_URL}/build/v86.wasm`,
        memory_size: 64 * 1024 * 1024,
        vga_memory_size: 8 * 1024 * 1024,
        screen_container: screenContainer,
        bios: { url: BIOS_URL },
        vga_bios: { url: VGA_BIOS_URL },
        hda: { url: DISK_IMAGE_URL, async: true },
        boot_order: 0x132,
        autostart: true,
    });

    emulator.add_listener("emulator-ready", () => {
        setStatus("Running");
        screenContainer.tabIndex = 0;
        screenContainer.focus();
    });
}

startButton.addEventListener("click", () => {
    if (emulator === null) {
        bootDemo();
    }
});

resetButton.addEventListener("click", () => {
    if (emulator !== null) {
        emulator.restart();
        setStatus("Restarting");
        screenContainer.focus();
    }
});

fullscreenButton.addEventListener("click", () => {
    if (screenContainer.requestFullscreen) {
        screenContainer.requestFullscreen();
    }
});
