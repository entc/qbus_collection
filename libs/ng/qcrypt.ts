import { AuthLoginCreds, AuthSessionItem } from '@qbus/auth_session';
import * as CryptoJS from 'crypto-js';
//-----------------------------------------------------------------------------

export interface Crypt4Params
{
    ha: string;
    id: string;
    da: string;
    wpid?: number;
    code?: string;
    vault?: string;
}

//-----------------------------------------------------------------------------

export class QCrypt {

    // for decryption
    private readonly decoder = new TextDecoder();
    private readonly encoder = new TextEncoder();

    //-------------------------------------------------------------------------

    static padding (str: string, max: number): string
    {
    	 return str.length < max ? QCrypt.padding ("0" + str, max) : str;
    }

    //-------------------------------------------------------------------------

    static b64_to_bytes (b64: string): Uint8Array
    {
        return Uint8Array.from(atob(b64), c => c.charCodeAt(0));
    }

    //-------------------------------------------------------------------------

    static bytes_to_hex (buffer: ArrayBuffer): string
    {
        return Array.from(new Uint8Array(buffer)).map(b => b.toString(16).padStart(2, '0')).join('');
    }

    //---------------------------------------------------------------------------

    public async sha256 (text: string): Promise<string>
    {
        // convert credentials string to UTF-8 bytes (Uint8Array)
        const data = this.encoder.encode(text);

        // compute SHA-256 hash
        const hash = await crypto.subtle.digest('SHA-256', data);

        // convert hash bytes to lowercase hexadecimal string
        return QCrypt.bytes_to_hex (hash);
    }

    //---------------------------------------------------------------------------

    public async derive_key (password: string, salt: Uint8Array, iterations: number): Promise<CryptoKey>
    {
        // imports the password as PBKDF2 input material (not a usable cryptographic key yet)
        const base_key = await crypto.subtle.importKey ("raw", this.encoder.encode(password), "PBKDF2", false, ["deriveKey"]);

        // taking a password-derived key (baseKey) and turning it into a real 256-bit AES-GCM encryption key using PBKDF2
        return crypto.subtle.deriveKey ({name: "PBKDF2", salt, iterations: iterations, hash: "SHA-256"}, base_key, {name: "AES-GCM", length: 256}, false, ["decrypt"]);
    }

    //---------------------------------------------------------------------------

    public async header_base64 (sitem: AuthSessionItem): Promise<string>
    {
        // get the linux time since 1970 in milliseconds
        const ha: string = QCrypt.padding (Date.now().toString(), 16);

        const da: string = await this.sha256 (ha + ":" + sitem.vsec);

        return btoa(JSON.stringify ({token: sitem.token, ha: ha, da: da}));
    }

    //---------------------------------------------------------------------------

    public async crypt4_authentication (creds: AuthLoginCreds): Promise<string>
    {
        const [hash1_buffer, user_buffer] = await Promise.all([
            crypto.subtle.digest("SHA-256", this.encoder.encode(creds.user + ":" + creds.pass)),
            crypto.subtle.digest("SHA-256", this.encoder.encode(creds.user))
        ]);

        // TODO: use a cached value here
        // convert from ArrayBuffer into hex string
        const hash1 = QCrypt.bytes_to_hex(hash1_buffer);
        const user = QCrypt.bytes_to_hex(user_buffer);

        // get the linux time since 1970 in milliseconds
        const ha: string = QCrypt.padding (Date.now().toString(), 16);

        const hash2 = await crypto.subtle.digest ("SHA-256", this.encoder.encode (ha + ":" + hash1));

        const params: Crypt4Params = {"ha": ha, "id": user, "da": QCrypt.bytes_to_hex(hash2)};

        if (creds.wpid)
        {
            params.wpid = creds.wpid;
        }

        if (creds.code)
        {
            const hash3 = await crypto.subtle.digest ("SHA-256", this.encoder.encode (creds.code));

            // use the new crypto interface
            params.code = QCrypt.bytes_to_hex(hash3);
        }

        if (creds.vault)
        {
            // TODO: replace this
            // ecrypt password with hash1
            // hash1 is known to the auth module
            params.vault = CryptoJS.AES.encrypt (creds.pass, hash1, { mode: CryptoJS.mode.CFB, padding: CryptoJS.pad.AnsiX923 }).toString();
        }

        // create a Object object as text
        return JSON.stringify (params);
    }

    //---------------------------------------------------------------------------

    private async decrypt_v1 (payload: Uint8Array, password: string): Promise<string>
    {
        const salt = payload.slice(1, 17);
        const iv = payload.slice(17, 29);
        const ciphertext = payload.slice(29);

        // derive AES-GCM CryptoKey from password + salt
        const key = await this.derive_key(password, salt, 100000);

        try
        {
            // decrypt
            const plaintext_buffer = await crypto.subtle.decrypt({name: "AES-GCM", iv}, key, ciphertext);

            return this.decoder.decode (plaintext_buffer);
        }
        catch
        {
            throw new Error("Invalid password or corrupted encrypted data");
        }
    }

    //---------------------------------------------------------------------------

    public async decrypt_item (payload_base64: string, password: string): Promise<string>
    {
        const payload = QCrypt.b64_to_bytes (payload_base64);

        if (payload.length <= 29)
        {
            throw new Error("Invalid encrypted payload");
        }

        switch (payload[0])
        {
            case 0x67:
            {
                return this.decrypt_v1 (payload, password);
            }
            default:
            {
                throw new Error(`Unsupported encryption version`);
            }
        }
    }

    //---------------------------------------------------------------------------

    public async encrypt_object (sitem: AuthSessionItem, params: object): Promise<string>
    {
        var h = JSON.stringify (params);

        return CryptoJS.AES.encrypt (h, sitem.vsec, { mode: CryptoJS.mode.CFB, padding: CryptoJS.pad.AnsiX923 }).toString();
    }

    //---------------------------------------------------------------------------
}
