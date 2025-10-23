document.getElementById("sendButton").addEventListener("click", async () => {
  const statusEl = document.getElementById("status");
  statusEl.textContent = "Відправляю...";
  try {
    const response = await fetch("/button", { method: "POST" });
    if (response.ok) {
      statusEl.textContent = "Повідомлення відправлено!";
    } else {
      statusEl.textContent = "Помилка запиту.";
    }
  } catch (err) {
    statusEl.textContent = "ESP32 недоступний.";
  }
});
