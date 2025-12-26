# Meshola - TODO & Task Tracking

**Last Updated:** December 25, 2024

---

## Current Sprint

**Focus:** Complete v0.2.0 - Views & Interaction

### In Progress 🚧

*No tasks currently in progress*

### Up Next 📋

| Task | Priority | Estimate | Notes |
|------|----------|----------|-------|
| ContactsView implementation | High | 2-3 hrs | Core UI for peer selection |
| ChannelsView implementation | High | 2-3 hrs | Core UI for channel selection |
| Profile Editor UI | High | 3-4 hrs | Full editing screen |
| Contact persistence | Medium | 1-2 hrs | JSON storage |
| Channel persistence | Medium | 1-2 hrs | JSON storage |
| JSON read implementation | Medium | 2-3 hrs | Currently write-only |

---

## Completed ✅

### December 25, 2024

- [x] Project structure and scaffolding
- [x] IProtocol interface definition
- [x] ProtocolRegistry implementation
- [x] MeshCoreProtocol stub
- [x] Profile struct and ProfileManager
- [x] MeshService singleton
- [x] MesholaApp with navigation
- [x] ChatView with full UI
- [x] MessageStore with JSON Lines
- [x] Unique node naming (Meshola-XXXX)
- [x] Per-profile keypair generation
- [x] Message persistence (append-on-receive)
- [x] Profile switching
- [x] Settings view placeholder
- [x] Documentation suite created
- [x] Fixed folder structure issue ({main garbage)

---

## Backlog

### High Priority

| Task | Category | Notes |
|------|----------|-------|
| ContactsView | UI | Peer list with selection |
| ChannelsView | UI | Channel list with management |
| Profile Editor | UI | Full editing interface |
| MeshCore integration | Protocol | Actual radio operations |
| Radio initialization | Hardware | SX1262 setup for T-Deck |

### Medium Priority

| Task | Category | Notes |
|------|----------|-------|
| Contact persistence | Storage | Save/load contacts per profile |
| Channel persistence | Storage | Save/load channels per profile |
| JSON read implementation | Storage | Parse saved JSON files |
| Thread-safe LVGL updates | Threading | Use tt_lvgl_lock() |
| Background mesh thread | Threading | Separate thread for radio loop |
| Loading indicators | UI | Show when operations in progress |
| Error dialogs | UI | User-friendly error messages |

### Low Priority

| Task | Category | Notes |
|------|----------|-------|
| Dark/light theme | UI | Theme toggle |
| Font size options | UI | Accessibility |
| Message search | Feature | Search through history |
| Export chat history | Feature | Backup messages |
| Notification sounds | Feature | Audio feedback |
| Keyboard shortcuts | UI | Improve navigation |

---

## Known Issues 🐛

| Issue | Severity | Status | Notes |
|-------|----------|--------|-------|
| JSON read not implemented | Medium | Open | Profiles/messages write but don't load |
| Thread mutex placeholder | Low | Open | Using nullptr, needs real mutex |
| Timestamp display simplified | Low | Open | Shows HH:MM from raw timestamp |

---

## Technical Debt

| Item | Priority | Notes |
|------|----------|-------|
| Replace placeholder mutexes | Medium | Use actual Tactility mutex |
| Proper error handling | Medium | Many functions silently fail |
| Memory leak audit | Low | Verify all allocations freed |
| Code comments | Low | Add doxygen-style comments |
| Unit tests | Low | No test framework yet |

---

## Documentation TODO

| Document | Status | Needs |
|----------|--------|-------|
| PROJECT_OVERVIEW.md | ✅ Complete | Keep updated |
| ARCHITECTURE.md | ✅ Complete | Update with new components |
| USER_GUIDE.md | ✅ Complete | Add screenshots when UI ready |
| DEVELOPER_GUIDE.md | ✅ Complete | Add more examples |
| API_REFERENCE.md | ✅ Complete | Keep in sync with code |
| CHANGELOG.md | ✅ Complete | Update with each change |
| ROADMAP.md | ✅ Complete | Review monthly |
| TODO.md | ✅ Complete | Update daily |

---

## Notes & Decisions

### December 25, 2024

**Profile System Design:**
- Each profile has unique keypair (not shared device identity)
- Profiles store all settings including protocol-specific ones
- Switching profiles reinitializes protocol completely
- Message history is per-profile (no merging)

**Message Persistence:**
- JSON Lines format chosen for append-only writes
- Messages saved immediately (no batching)
- Acceptable for mesh traffic volume
- Flash wear is non-issue for this use case

**Protocol Abstraction:**
- IProtocol interface designed for future protocols
- Feature detection allows UI adaptation
- Runtime switching without recompile
- ProtocolRegistry pattern for extensibility

---

## Questions & Research Needed

| Question | Status | Notes |
|----------|--------|-------|
| MeshCore API stability | Open | Check before deep integration |
| Tactility thread API | Open | Verify Thread/Mutex usage |
| SX1262 pin mapping | Known | T-Deck pins documented |
| Flash storage limits | Open | Check partition size |
| LVGL memory usage | Open | Monitor during development |

---

## Meeting Notes

*No meetings scheduled*

---

## Quick Reference

### File Locations
```
Source:  Apps/Meshola/main/Source/
Docs:    MesholaDocs/
Storage: /data/meshola/
```

### Build Commands
```bash
cd TactilityApps/Apps/Meshola
idf.py build
idf.py -p /dev/ttyUSB0 flash monitor
```

### Git Workflow
```bash
git add Apps/Meshola
git commit -m "description"
git push
```

---

*This document is updated as tasks progress. Check daily for current status.*
