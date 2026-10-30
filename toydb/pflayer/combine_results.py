import csv

files = [
    ("Locality", "benchmark_locality.csv"),
    ("Sequential", "benchmark_sequential.csv")
]

rows = []

for workload, filename in files:
    with open(filename, "r") as f:
        reader = csv.DictReader(f)

        for row in reader:
            row = {"Workload": workload, **row}
            rows.append(row)

with open("task1_results.csv", "w", newline="") as f:
    writer = csv.DictWriter(f, fieldnames=rows[0].keys())
    writer.writeheader()
    writer.writerows(rows)

print("Created task1_results.csv")
print("Total experiments:", len(rows))