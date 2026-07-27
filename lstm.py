import numpy as np
import pandas as pd
import tensorflow as tf
from tensorflow.keras.models import Model
from tensorflow.keras.layers import Input, LSTM, Dense, Dropout

# 1. SIMULASI DATA DAN TRANSFORMASI SIN/COS
# Anggap df_api adalah data hourly yang Anda unduh dari Open-Meteo
np.random.seed(42)
date_range = pd.date_range(start="2021-01-01", end="2025-12-31 23:00:00", freq="h")
n_samples = len(date_range)

df = pd.DataFrame({
    'temperature': np.random.normal(27, 2, n_samples),
    'humidity': np.random.uniform(60, 95, n_samples),
    'rain': np.random.exponential(0.5, n_samples) # Target utama
}, index=date_range)

# Membuat komponen Sin/Cos untuk Hari dalam Setahun (Musiman)
df['day_of_year'] = df.index.dayofyear
df['sin_year'] = np.sin(2 * np.pi * df['day_of_year'] / 365.25)
df['cos_year'] = np.cos(2 * np.pi * df['day_of_year'] / 365.25)

# Fitur yang digunakan untuk latihan
features = ['temperature', 'humidity', 'rain', 'sin_year', 'cos_year']
data_matrix = df[features].values

# 2. STRUKTUR SLIDING WINDOW UNTUK MULTI-HEAD TARGET
def create_multi_horizon_dataset(data, rain_idx, lookback=720):
    """
    lookback = 720 jam (30 hari ke belakang)
    Target Pendek: 72 jam ke depan (per jam)
    Target Menengah: hari ke-5 s.d hari ke-16 (12 hari, diakumulasi harian)
    Target Panjang: minggu ke-1 s.d minggu ke-4 (4 minggu, diakumulasi mingguan)
    """
    X, y_short, y_med, y_long = [], [], [], []
    
    # Batas akhir iterasi agar window target tidak out of bounds (~30 hari ke depan)
    max_forecast_steps = 24 * 30 
    
    for i in range(lookback, len(data) - max_forecast_steps):
        # Input: 30 hari ke belakang
        X.append(data[i-lookback:i])
        
        # Ekstrak data hujan masa depan untuk target
        future_rain = data[i:i+max_forecast_steps, rain_idx]
        
        # a. Jangka Pendek (1-72 jam ke depan, per jam)
        y_short.append(future_rain[0:72])
        
        # b. Jangka Menengah (Hari 5-16, total harian -> 12 nilai)
        # Hari ke-5 dimulai dari jam ke-96 (4 hari * 24 jam) hingga jam ke-384
        med_part = future_rain[24*4 : 24*16]
        med_daily = [np.sum(med_part[j:j+24]) for j in range(0, len(med_part), 24)]
        y_med.append(med_daily)
        
        # c. Jangka Panjang (Minggu 1-4, total mingguan -> 4 nilai)
        # Total akumulasi per 7 hari dari jam ke-0 hingga jam ke-672 (28 hari)
        long_part = future_rain[0 : 24*28]
        long_weekly = [np.sum(long_part[j:j+(24*7)]) for j in range(0, len(long_part), 24*7)]
        y_long.append(long_weekly)
        
    return np.array(X), np.array(y_short), np.array(y_med), np.array(y_long)

# Cari indeks kolom 'rain'
rain_column_index = features.index('rain')

# Buat dataset (Disarankan lakukan Scaling/MinMaxScaler pada data_matrix terlebih dahulu)
X, y_s, y_m, y_l = create_multi_horizon_dataset(data_matrix, rain_column_index)

# 3. MEMBANGUN ARSITEKTUR LSTM MULTI-HEAD OUTPUT
input_layer = Input(shape=(X.shape[1], X.shape[2]), name='Input_Historis')

# Backbone LSTM untuk membaca representasi pola cuaca
lstm_1 = LSTM(64, return_sequences=True)(input_layer)
dropout_1 = Dropout(0.2)(lstm_1)
lstm_2 = LSTM(32)(dropout_1)

# Cabang 1: Output Jangka Pendek (72 Jam ke depan)
dense_short = Dense(64, activation='relu')(lstm_2)
output_short = Dense(72, name='Output_Pendek')(dense_short)

# Cabang 2: Output Jangka Menengah (12 Hari ke depan, Akumulasi Harian)
dense_med = Dense(32, activation='relu')(lstm_2)
output_med = Dense(12, name='Output_Menengah')(dense_med)

# Cabang 3: Output Jangka Panjang (4 Minggu ke depan, Akumulasi Mingguan)
dense_long = Dense(16, activation='relu')(lstm_2)
output_long = Dense(4, name='Output_Panjang')(dense_long)

# Satukan ke dalam satu Model
model = Model(inputs=input_layer, outputs=[output_short, output_med, output_long])

# Kompilasi dengan loss MSE untuk tiap cabang
model.compile(
    optimizer='adam',
    loss={
        'Output_Pendek': 'mse',
        'Output_Menengah': 'mse',
        'Output_Panjang': 'mse'
    },
    loss_weights={
        'Output_Pendek': 1.0, 
        'Output_Menengah': 0.8, # Bobot bisa disesuaikan dengan prioritas akurasi
        'Output_Panjang': 0.5
    }
)

model.summary()

# 4. PROSES TRAINING
# model.fit(X, {'Output_Pendek': y_s, 'Output_Menengah': y_m, 'Output_Panjang': y_l}, epochs=20, batch_size=64)
