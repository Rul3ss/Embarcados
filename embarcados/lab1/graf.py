import matplotlib.pyplot as plt

# Lê os dados do arquivo
x = []
y = []

with open('senoide.txt', 'r') as arquivo:
    for linha in arquivo:
        valores = linha.split()
        x.append(float(valores[0]))
        y.append(float(valores[1]))

# Cria o gráfico
plt.figure(figsize=(12, 6))
plt.plot(x, y, 'b-', linewidth=2, label='Série de Taylor (10 termos)')
plt.xlabel('x (radianos)')
plt.ylabel('sen(x)')
plt.title('Senoide gerada pela Série de Taylor')
plt.grid(True, alpha=0.3)
plt.legend()
plt.axhline(y=0, color='k', linewidth=0.5)
plt.axvline(x=0, color='k', linewidth=0.5)

# Salva o gráfico como imagem
plt.savefig('grafico_senoide.png', dpi=300, bbox_inches='tight')

# Mostra o gráfico
plt.show()

print("Gráfico salvo como 'grafico_senoide.png'")