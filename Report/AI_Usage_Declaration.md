# AI Usage Declaration

**Course:** CS202 — Programming Techniques
**Project:** Apple Knight Adventure (2D action-platformer, C++17 / raylib / LDtk)
**Repository:** `CS202-FinalProject`
**Period covered:** 2026-06-08 → 2026-08-31

---

## About this document

Our team used an AI coding assistant during development. We did not keep verbatim
chat transcripts for the whole project, so **this log is a reconstruction**: each
entry was rebuilt from the commit that the conversation produced, and records the
substance of what we asked and what we got rather than the exact wording. The
commit hashes and messages referenced in each entry are real and can be checked
against `git log`.

Everything below was reviewed, compiled, and tested by us before being committed.
Where the assistant produced something wrong, that is recorded too — several
entries are follow-up prompts after a first answer failed.

**Division of work.** The project was split by layer. Nguyễn Anh Kiệt owned the
View/UI/rendering layer, the animation system, the boss system, the LDtk and Map
Builder tooling, and the audio system. Nguyễn Trọng Tiến owned the Model layer and
entity hierarchy, level design and map authoring, gameplay systems (checkpoints,
save data, elemental combat, buffs), and gameplay bug-fixing.

| Member | Primary area | Commits |
|---|---|---|
| Nguyễn Anh Kiệt | View / rendering, UI, animation, boss system, tooling, audio | ~158 |
| Nguyễn Trọng Tiến | Model, level design, gameplay systems, save data, bug-fixing | ~42 |

---

