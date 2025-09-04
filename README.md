# Cybersecurity Challenges by Depp1135

Welcome to my collection of **original CTF-style cybersecurity challenges**, all created by me (Antonio / Depp1135).  
This repository is both a personal portfolio and a playground for events or just-for-fun challenges.

---

## 🕹️ About
I design and implement challenges to replicate real-world security issues while keeping them fun and solvable in a CTF setting.  
This repo serves two purposes:
- Showcase my ability to **create challenges from scratch** (for future job opportunities).
- Provide reproducible challenges that can be reused in events or for personal training.

Categories include:
- `pwn`
- `misc`
- `crypto`
- `web`
- `forensics`

---

## 📂 Repository Structure
```
.
├── event/
│   ├── pwn/
│   │   └── buffer-overflow-101/
│   └── crypto/
│       └── weak-rsa/
└── fun/
    └── misc/
        └── realtime-crypto-market/
```

Each challenge folder contains:
- `challenge/` → source code / Dockerfile / setup scripts  
- `README.md` → description and intended learning outcome  
- `solution/` → reference solve script or writeup (optional, hidden during competitions)  

---

## 🚀 Getting Started
Clone the repository and spin up a challenge with Docker:

```bash
git clone https://github.com/Depp1135/depp1135-ctf-challenges.git
cd depp1135-ctf-challenges/event/pwn/buffer-overflow-101
docker compose up --build
```

Then connect via browser / netcat depending on the challenge instructions.

---

## 🎯 Goals of this Repository
- Demonstrate my ability to **design security challenges from scratch**  
- Showcase knowledge in **low-level systems, cryptography, and web security**  
- Contribute to the **CTF and security learning community**  
- Build a portfolio of practical, reproducible work  

---

## 📬 Contact
- 👨‍🎓 Bachelor student in Computer Engineering @ Politecnico di Torino  
- 🔐 Member of **Pwnthem0le** CTF team  
- 🐙 GitHub: [Depp1135](https://github.com/thedeppside)  
- 💼 LinkedIn: *[Antonio Galasso](https://www.linkedin.com/in/antonio-galasso-45394b35b)*  

---

## 🏴 Flag Meme
```
flag{th1s_r3po_1s_my_p3rson4l_cTF_f4ct0ry}
```

---

> _“Hope you'll have fun. 👾”_
