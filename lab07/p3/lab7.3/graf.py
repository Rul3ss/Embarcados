import matplotlib.pyplot as plt

# Lê o arquivo
with open("dados.txt", "r") as f:
    dados = f.read().strip()

valores = list(map(int, dados.split(",")))
tempo = list(range(len(valores)))  # 0, 1, 2, ... ms



plt.figure(figsize=(12, 4))
plt.plot(tempo, valores, linewidth=0.8, color="royalblue")
plt.title("Leitura ADC - 1000 amostras em 1 segundo")
plt.xlabel("Tempo (ms)")
plt.ylabel("Valor ADC (0 - 4095)")
plt.ylim(-50, 5000)
plt.grid(True, alpha=0.4)
plt.tight_layout()
plt.savefig("grafico.png", dpi=150)
print(f"Total de amostras: {len(valores)}")
print("Gráfico salvo em grafico.png")
