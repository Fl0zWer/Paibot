#!/usr/bin/env python3
"""
Paibot Integrity Validation Script

Performs basic smoke tests and validation checks for the Paibot mod.
This script can be run as part of CI/CD or manually for development.
"""

import json
import os
import sys
import hashlib
from pathlib import Path

def validate_mod_json():
    """Validate mod.json structure and required fields"""
    print("Validating mod.json...")
    
    if not os.path.exists("mod.json"):
        print("❌ mod.json not found")
        return False
        
    try:
        with open("mod.json", "r") as f:
            data = json.load(f)
            
        required_fields = ["id", "name", "version", "min-game-version", "geode", "dependencies"]
        missing = [field for field in required_fields if field not in data]
        
        if missing:
            print(f"❌ Missing required fields: {missing}")
            return False
            
        # Check settings structure
        if "settings" not in data:
            print("❌ Missing settings section")
            return False
            
        print("✅ mod.json validation passed")
        return True
        
    except json.JSONDecodeError as e:
        print(f"❌ mod.json JSON parsing failed: {e}")
        return False
    except Exception as e:
        print(f"❌ mod.json validation failed: {e}")
        return False

def validate_resource_structure():
    """Validate resource directory structure"""
    print("Validating resource structure...")
    
    required_dirs = ["resources", "resources/presets", "resources/textures", "resources/sounds"]
    
    for dir_path in required_dirs:
        if not os.path.isdir(dir_path):
            print(f"❌ Missing directory: {dir_path}")
            return False
            
    # Check for manifest
    manifest_path = "resources/manifest.json"
    if not os.path.exists(manifest_path):
        print(f"❌ Missing resource manifest: {manifest_path}")
        return False
        
    try:
        with open(manifest_path, "r") as f:
            manifest = json.load(f)
            
        required_manifest_fields = ["version", "resources"]
        missing = [field for field in required_manifest_fields if field not in manifest]
        
        if missing:
            print(f"❌ Manifest missing fields: {missing}")
            return False
            
        print("✅ Resource structure validation passed")
        return True
        
    except Exception as e:
        print(f"❌ Resource structure validation failed: {e}")
        return False

def calculate_resource_hashes():
    """Calculate and display resource hashes for integrity verification"""
    print("Calculating resource hashes...")
    
    resource_dirs = ["resources/presets", "resources/textures", "resources/sounds"]
    total_hash = hashlib.sha256()
    
    for dir_path in resource_dirs:
        dir_hash = hashlib.sha256()
        
        if os.path.exists(dir_path):
            for root, dirs, files in os.walk(dir_path):
                for file in sorted(files):  # Sort for consistent hash
                    if not file.startswith('.'):  # Skip hidden files
                        file_path = os.path.join(root, file)
                        with open(file_path, "rb") as f:
                            file_content = f.read()
                            file_hash = hashlib.sha256(file_content).hexdigest()
                            dir_hash.update(file_content)
                            print(f"  {file_path}: {file_hash[:16]}...")
            
            total_hash.update(dir_hash.digest())
            print(f"📁 {dir_path}: {dir_hash.hexdigest()[:16]}...")
    
    print(f"🏷️  Total hash: {total_hash.hexdigest()[:16]}...")
    return total_hash.hexdigest()

def validate_cmake_structure():
    """Validate CMakeLists.txt structure"""
    print("Validating CMakeLists.txt...")
    
    if not os.path.exists("CMakeLists.txt"):
        print("❌ CMakeLists.txt not found")
        return False
        
    try:
        with open("CMakeLists.txt", "r") as f:
            content = f.read()
            
        required_elements = [
            "cmake_minimum_required",
            "project(PaibotGeodeBase",
            "GEODE_REQUIRED_VERSION",
            "setup_geode_mod"
        ]
        
        missing = [elem for elem in required_elements if elem not in content]
        
        if missing:
            print(f"❌ CMakeLists.txt missing elements: {missing}")
            return False
            
        print("✅ CMakeLists.txt validation passed")
        return True
        
    except Exception as e:
        print(f"❌ CMakeLists.txt validation failed: {e}")
        return False

def validate_ci_config():
    """Validate GitHub Actions CI configuration"""
    print("Validating CI configuration...")
    
    workflow_path = ".github/workflows/build.yml"
    if not os.path.exists(workflow_path):
        print(f"❌ CI workflow not found: {workflow_path}")
        return False
        
    try:
        with open(workflow_path, "r") as f:
            content = f.read()
            
        required_jobs = ["build-windows", "build-linux", "integrity-check"]
        missing_jobs = [job for job in required_jobs if job not in content]
        
        if missing_jobs:
            print(f"❌ CI missing jobs: {missing_jobs}")
            return False
            
        print("✅ CI configuration validation passed")
        return True
        
    except Exception as e:
        print(f"❌ CI configuration validation failed: {e}")
        return False

def main():
    """Run all validation checks"""
    print("🚀 Starting Paibot integrity validation...")
    print("=" * 50)
    
    checks = [
        validate_mod_json,
        validate_resource_structure,
        validate_cmake_structure,
        validate_ci_config
    ]
    
    passed = 0
    failed = 0
    
    for check in checks:
        if check():
            passed += 1
        else:
            failed += 1
        print()
    
    # Calculate resource hashes
    total_hash = calculate_resource_hashes()
    print()
    
    print("=" * 50)
    print(f"📊 Results: {passed} passed, {failed} failed")
    
    if failed == 0:
        print("🎉 All validation checks passed!")
        print(f"📋 Resource integrity hash: {total_hash[:32]}")
        return 0
    else:
        print("❌ Some validation checks failed")
        return 1

if __name__ == "__main__":
    sys.exit(main())