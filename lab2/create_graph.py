import matplotlib.pyplot as plt
import matplotlib
matplotlib.use('TkAgg')  # Используем стандартный бэкенд TkAgg
import pandas
import platform
import subprocess
import sys

# Функция для установки библиотеки
def install(package):
    try:
        subprocess.check_call([sys.executable, "-m", "pip", "install", package])
        return True  # Установка успешна
    except Exception as e:
        print(f"Ошибка при установке библиотеки {package}: {e}")
        return False  # Установка не удалась

# Попытка импортировать cpuinfo, если не удается — установить
try:
    import cpuinfo
except ImportError:
    print("Библиотека cpuinfo не установлена. Пытаемся установить...")
    if not install('py-cpuinfo'):
        print("Не удалось установить библиотеку cpuinfo. Продолжаем без неё.")
        cpuinfo = None  # Указываем, что библиотека недоступна
    else:
        import cpuinfo

# Создаем график
fig, ax = plt.subplots(figsize=(8, 6))  # Фиксированный размер графика

data = pandas.read_csv("output.csv")
length = data.shape[0]

t = data[0:0 + length]["test"]
scalar = data[0:0 + length]["scalar"]
vector = data[0:0 + length]["vector"]

ax.plot(t, [float(i) for i in scalar], label="Время выполнения scalar", c="blue")
ax.plot(t, [float(i) for i in vector], label="Время выполнения vector", c="red")


ax.set_xlabel('Тесты')
ax.set_ylabel('Время выполнения, мс')
ax.set_xticks(t)
ax.grid(True, axis='y', linestyle='--', alpha=0.7)
ax.legend()

# Если библиотека cpuinfo доступна, получаем информацию о процессоре
if cpuinfo is not None:
    try:
        cpu_info = cpuinfo.get_cpu_info()
        processor_info = f"""
        ПАРАМЕТРЫ ТАЧКИ: 
        
        Model name:                         {cpu_info.get('brand_raw', 'N/A')}
        Core(s) per socket:                 {cpu_info.get('count', 'N/A')}
        Socket(s):                          {platform.processor()}
        CPU max MHz:                        {cpu_info.get('hz_advertised_friendly', 'N/A')}
        CPU min MHz:                        {cpu_info.get('hz_actual_friendly', 'N/A')}
        """

        print(processor_info)
    except Exception as e:
        print(f"Ошибка при получении информации о процессоре: {e}")
        processor_info = None
else:
    print("Информация о процессоре недоступна, так как библиотека cpuinfo не установлена.")
    processor_info = None

# Если информация о процессоре доступна, добавляем её на график
if processor_info is not None:
    plt.figtext(0.1, 0.02, processor_info, wrap=True, horizontalalignment='left', fontsize=8)
    plt.tight_layout()  # Автоматически регулирует отступы
    plt.subplots_adjust(bottom=0.3)  # Увеличиваем место внизу для текста

# Сохраняем график в файл
plt.savefig("output_plot.png", dpi=300, bbox_inches='tight')  # Сохраняем с фиксированным размером и разрешением

# Показываем график
plt.show()