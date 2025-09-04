#!/usr/bin/env python3
from datetime import datetime, timezone

def minutes(hhmm: str) -> int:
    hh, mm = hhmm.split(":")
    h = int(hh); m = int(mm)
    if not (0 <= h < 24 and 0 <= m < 60):
        raise ValueError("Invalid time format HH:MM (00<=HH<24, 00<=MM<60)")
    return h*60 + m

def norm_minutes(m: int) -> int:
    m %= 1440
    return m

def wrap_lon(lon: float) -> float:
    # Normalize to [-180, 180]
    while lon < -180.0:
        lon += 360.0
    while lon > 180.0:
        lon -= 360.0
    return lon

def compute_lon(target_hhmm: str) -> float:
    utc_hhmm = None
    T = minutes(target_hhmm)
    if utc_hhmm is None:
        now = datetime.now(timezone.utc)
        U = now.hour*60 + now.minute
    else:
        U = minutes(utc_hhmm)
    # local_from_lon ≈ UTC + lon*4min/°  => lon ≈ (T - U)/4
    delta = ((T - U + 720) % 1440) - 720  # put in [-720, 719]
    lon = delta / 4.0
    return wrap_lon(lon)

def main(target):

    lon = compute_lon(target)
    lat = 0.0
    if lat < -90 or lat > 90:
        raise SystemExit("Latitude must be within [-90,90].")

    # print(f"lat={lat:.6f}")
    # print(f"lon={lon:.6f}")
    return (lat, lon)

if __name__ == "__main__":
    main()
